"""
Multi-Round Retrieval Engine — core orchestrator for AgenticDB.

Pipeline:
  1. QueryPlanner → RetrievalPlan
  2. Execute search steps (embed + DeepVector search)
  3. ResultEvaluator → quality score
  4. If insufficient: QueryReformulator → new queries → step 2
  5. If sufficient: AnswerGenerator → final answer
"""

import logging
from typing import Any, Dict, List, Optional

import httpx

from ..config import AgenticDBConfig
from ..llm.router import LLMRouter
from ..llm.prompts import ANSWER_SYSTEM, RESPONSE_FORMATTER
from ..embedding.service import EmbeddingService
from .query_planner import QueryPlanner
from .result_evaluator import ResultEvaluator
from .query_reformulator import QueryReformulator
from .strategy import RetrievalPlan

logger = logging.getLogger(__name__)


class RetrievalResult:
    """Complete retrieval result with answer, documents, and metadata."""

    def __init__(
        self,
        answer: str,
        documents: List[Dict[str, Any]],
        plan: RetrievalPlan,
        rounds: int,
        quality_score: float,
        all_queries_tried: List[str],
    ):
        self.answer = answer
        self.documents = documents
        self.plan = plan
        self.rounds = rounds
        self.quality_score = quality_score
        self.all_queries_tried = all_queries_tried

    def to_dict(self) -> Dict[str, Any]:
        return {
            "answer": self.answer,
            "documents": self.documents,
            "strategy": self.plan.strategy.value,
            "rounds": self.rounds,
            "quality_score": self.quality_score,
            "queries_tried": self.all_queries_tried,
            "plan_reasoning": self.plan.reasoning,
        }


class MultiRoundEngine:
    """Orchestrate plan → search → evaluate → reformulate → answer."""

    def __init__(self, config: AgenticDBConfig, llm: LLMRouter):
        self.config = config
        self.llm = llm
        self.planner = QueryPlanner(llm, config.default_collection)
        self.evaluator = ResultEvaluator(llm, config.quality_threshold)
        self.reformulator = QueryReformulator(llm)
        self._client: Optional[httpx.AsyncClient] = None
        self._embedder: Optional[EmbeddingService] = None

    async def _ensure_client(self):
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    async def _ensure_embedder(self):
        if self._embedder is None:
            self._embedder = EmbeddingService(self.config.embedding)

    async def retrieve(
        self, question: str, collection: Optional[str] = None
    ) -> RetrievalResult:
        await self._ensure_client()
        await self._ensure_embedder()
        coll = collection or self.config.default_collection

        plan = await self.planner.plan(question, coll)
        logger.info("Plan: %s", plan.summary())

        all_results: List[Dict[str, Any]] = []
        all_queries: List[str] = []
        all_seen_ids = set()
        quality_score = 0.0
        round_num = 0

        # Prefer any filter from the planned steps for later reformulation rounds
        active_filter: Optional[Dict[str, Any]] = None
        for step in plan.steps:
            if step.filter:
                active_filter = step.filter
                break

        current_queries: List[str] = []
        use_plan_steps = True

        for round_num in range(1, self.config.max_rounds + 1):
            logger.info("=== Round %d/%d ===", round_num, self.config.max_rounds)
            round_results: List[Dict[str, Any]] = []

            if use_plan_steps:
                for step in plan.steps:
                    if step.query in all_queries and not step.filter:
                        continue
                    all_queries.append(step.query)
                    results = await self._execute_search(
                        step.query,
                        step.collection or coll,
                        k=step.k,
                        filter=step.filter,
                    )
                    for r in results:
                        if r["id"] not in all_seen_ids:
                            all_seen_ids.add(r["id"])
                            round_results.append(r)
                            all_results.append(r)
                use_plan_steps = False
            else:
                for query in current_queries:
                    if query in all_queries:
                        continue
                    all_queries.append(query)
                    results = await self._execute_search(
                        query, coll, filter=active_filter
                    )
                    for r in results:
                        if r["id"] not in all_seen_ids:
                            all_seen_ids.add(r["id"])
                            round_results.append(r)
                            all_results.append(r)

            if not round_results:
                logger.info("No new results in round %d", round_num)
                if round_num == 1 and not all_results:
                    pass  # still evaluate empty → continue/reformulate
                else:
                    break

            eval_result = await self.evaluator.evaluate(
                question, all_results, round_num, self.config.max_rounds
            )
            quality_score = eval_result.score
            logger.info("Quality: %s", eval_result)

            if not eval_result.should_continue:
                logger.info("Quality threshold met, stopping")
                break

            if round_num >= self.config.max_rounds:
                logger.info("Max rounds reached")
                break

            current_queries = await self.reformulator.reformulate(
                question,
                all_queries,
                eval_result.feedback,
                strategy_hint=plan.strategy.value,
            )
            logger.info("New queries for next round: %s", current_queries)

        answer = await self._generate_answer(question, all_results)

        return RetrievalResult(
            answer=answer,
            documents=all_results[: self.config.top_k_final],
            plan=plan,
            rounds=round_num if round_num > 0 else 0,
            quality_score=quality_score,
            all_queries_tried=all_queries,
        )

    async def _execute_search(
        self,
        query: str,
        collection: str,
        k: Optional[int] = None,
        filter: Optional[Dict[str, Any]] = None,
    ) -> List[Dict[str, Any]]:
        url = f"{self.config.deepvector_url}/search"
        payload: Dict[str, Any] = {
            "vector": await self._embed_query(query),
            "k": k or self.config.max_results_per_round,
            "collection": collection,
        }
        if filter:
            payload["filter"] = filter

        try:
            resp = await self._client.post(url, json=payload)
            resp.raise_for_status()
            data = resp.json()

            results = []
            for item in data.get("results", []):
                text = item.get("text", "")
                tags = item.get("tags", "")
                timestamp = item.get("timestamp", 0)
                if not text and not tags:
                    meta = await self._fetch_meta(item["id"], collection)
                    text = meta.get("text", "")
                    tags = meta.get("tags", "")
                    timestamp = meta.get("timestamp", 0)
                results.append(
                    {
                        "id": item["id"],
                        "distance": item["distance"],
                        "text": text,
                        "tags": tags,
                        "timestamp": timestamp,
                    }
                )
            return results

        except Exception as e:
            logger.error("Search failed for query '%s': %s", query, e)
            return []

    async def _embed_query(self, text: str) -> List[float]:
        await self._ensure_embedder()
        return await self._embedder.embed_single(text)

    async def _fetch_meta(
        self, vector_id: int, collection: str
    ) -> Dict[str, Any]:
        try:
            resp = await self._client.get(
                f"{self.config.deepvector_url}/vectors/{vector_id}/meta",
                params={"collection": collection},
            )
            if resp.status_code == 200:
                return resp.json()
        except Exception as e:
            logger.warning("meta fetch failed id=%s: %s", vector_id, e)
        return {"text": "", "tags": "", "timestamp": 0}

    async def _generate_answer(
        self, question: str, results: List[Dict[str, Any]]
    ) -> str:
        if not results:
            return "No relevant documents found."

        docs_text = "\n\n".join(
            f"[Doc {i+1}] (id={r['id']}, distance={r['distance']:.4f})\n"
            f"{r.get('text', 'No text available')}"
            for i, r in enumerate(results[:10])
        )

        prompt = RESPONSE_FORMATTER.format(
            question=question, documents=docs_text
        )

        messages = [
            {"role": "system", "content": ANSWER_SYSTEM},
            {"role": "user", "content": prompt},
        ]

        response = await self.llm.chat(messages)
        return response.content or "Unable to generate answer."

    async def close(self):
        if self._client and not self._client.is_closed:
            await self._client.aclose()
        if self._embedder:
            await self._embedder.close()
            self._embedder = None
