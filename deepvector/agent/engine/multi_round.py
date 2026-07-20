"""
澶氳疆妫€绱㈠紩鎿?/ Multi-Round Retrieval Engine 鈥?AgenticDB 鐨勬牳蹇冪紪鎺掑櫒銆?
鏍稿績娴佺▼ / Core Pipeline:
  1. QueryPlanner 鈫?RetrievalPlan (绛栫暐閫夋嫨)
  2. 鎵ц鎼滅储姝ラ (宓屽叆 + DeepVector 鎼滅储)
  3. ResultEvaluator 鈫?璐ㄩ噺璇勫垎
  4. 濡傛灉璐ㄩ噺涓嶈冻: QueryReformulator 鈫?鏂版煡璇?鈫?鍥炲埌姝ラ2
  5. 濡傛灉璐ㄩ噺杈炬爣: AnswerGenerator 鈫?鏈€缁堢瓟妗?
鏁版嵁娴?/ Data Flow:
  User Question
    鈫?  [QueryPlanner]
    鈫?RetrievalPlan
  [MultiRoundEngine.Round 1]
    鈹溾攢鈹€ embed question
    鈹溾攢鈹€ search DeepVector
    鈹斺攢鈹€ collect results
    鈫?  [ResultEvaluator] 鈫?LLM
    鈹溾攢鈹€ score 鈮?0.7 鈫?[AnswerGenerator] 鈫?Final Answer
    鈹斺攢鈹€ score < 0.7
         鈫?  [QueryReformulator] 鈫?LLM
    鈫?new_queries
  [MultiRoundEngine.Round 2] ...寰幆

璁捐鍘熷垯 / Design Principles:
  - 寮傛鍏ㄩ摼璺? 鎵€鏈?IO 閮芥槸 async (LLM 璋冪敤 + 鏁版嵁搴撴煡璇?
  - 瀹归敊璁捐: 姣忚疆鎼滅储澶辫触涓嶉樆鏂暣浣撴祦绋?  - 璐ㄩ噺椹鹃┒: 缁撴灉璐ㄩ噺鍐冲畾鏄惁缁х画妫€绱? 鑰岄潪鍥哄畾杞暟
  - 璧勬簮淇濇姢: max_rounds 闃叉鏃犻檺寰幆
"""

import json
import logging
from typing import Any, Dict, List, Optional

import httpx

from ..config import AgenticDBConfig
from ..llm.router import LLMRouter
from ..llm.prompts import ANSWER_SYSTEM, RESPONSE_FORMATTER
from .query_planner import QueryPlanner
from .result_evaluator import ResultEvaluator
from .query_reformulator import QueryReformulator
from .strategy import RetrievalPlan, SearchStrategy

logger = logging.getLogger(__name__)


class RetrievalResult:
    """
    瀹屾暣妫€绱㈢粨鏋?/ Complete retrieval result.

    鍖呭惈鏈€缁堢瓟妗堛€佹绱㈠埌鐨勬枃妗ｃ€佷娇鐢ㄧ殑璁″垝銆佽疆娆′俊鎭拰璐ㄩ噺璇勫垎銆?    鎵€鏈夊瓧娈靛彲搴忓垪鍖栦负 JSON 鐢ㄤ簬 API 鍝嶅簲銆?
    Attributes:
        answer: LLM 鐢熸垚鐨勬渶缁堢瓟妗堟枃鏈?        documents: 妫€绱㈠埌鐨勬枃妗ｅ垪琛?        plan: 浣跨敤鐨勬绱㈣鍒?        rounds: 瀹為檯鎵ц鐨勮疆娆℃暟
        quality_score: 鏈€缁堣川閲忚瘎鍒?        all_queries_tried: 鎵€鏈夊皾璇曡繃鐨勬煡璇?    """

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
        """
        搴忓垪鍖栦负瀛楀吀 / Serialize to dictionary.

        鐢ㄤ簬 JSON 搴忓垪鍖?(API 鍝嶅簲鍜屾棩蹇?銆?
        Returns:
            鍖呭惈鎵€鏈夊瓧娈电殑瀛楀吀
        """
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
    """
    澶氳疆妫€绱㈠紩鎿?/ Multi-Round Retrieval Engine.

    缂栨帓瀹屾暣鐨勬绱㈡祦绋? 瑙勫垝 鈫?鎵ц 鈫?璇勪及 鈫?閲嶆瀯(濡傛湁闇€瑕? 鈫?鍥炵瓟銆?
    杩欐槸 AgenticDB 鐨勬牳蹇冪被, 涓茶仈浜嗘墍鏈夌粍浠躲€?
    鐢ㄦ硶 / Usage:
        engine = MultiRoundEngine(config, llm)
        result = await engine.retrieve("What is RAG and how does it work?")
        print(result.answer)
        print(f"Strategy: {result.plan.strategy.value}")
        print(f"Rounds: {result.rounds}")
    """

    def __init__(self, config: AgenticDBConfig, llm: LLMRouter):
        """
        鍒濆鍖栧杞绱㈠紩鎿?/ Initialize multi-round engine.

        Args:
            config: AgenticDB 鍏ㄥ眬閰嶇疆
            llm: LLM 璺敱鍣ㄥ疄渚?        """
        self.config = config
        self.llm = llm
        self.planner = QueryPlanner(llm, config.default_collection)
        self.evaluator = ResultEvaluator(llm, config.quality_threshold)
        self.reformulator = QueryReformulator(llm)
        self._client: Optional[httpx.AsyncClient] = None

    async def _ensure_client(self):
        """纭繚 DeepVector HTTP 瀹㈡埛绔凡鍒濆鍖?/ Ensure HTTP client initialized."""
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    async def retrieve(
        self, question: str, collection: Optional[str] = None
    ) -> RetrievalResult:
        """
        鎵ц澶氳疆妫€绱?/ Execute multi-round retrieval.

        瀹屾暣娴佺▼:
          1. QueryPlanner 鐢熸垚妫€绱㈣鍒?          2. 鎵ц绗竴杞悳绱?(鎸夎鍒掔殑 steps)
          3. ResultEvaluator 璇勪及缁撴灉璐ㄩ噺
          4. 璐ㄩ噺涓嶈冻 鈫?QueryReformulator 鈫?鎵ц鏂拌疆娆?          5. 璐ㄩ噺杈炬爣鎴栬秴杞 鈫?鐢熸垚鏈€缁堢瓟妗?
        Args:
            question: 鐢ㄦ埛鐨勮嚜鐒惰瑷€闂
            collection: 闆嗗悎鍚嶇О (鍙€?

        Returns:
            RetrievalResult 鍖呭惈绛旀銆佹枃妗ｅ拰鍏冧俊鎭?        """
        await self._ensure_client()
        coll = collection or self.config.default_collection

        # Step 1: 鏌ヨ瑙勫垝 / Generate retrieval plan
        plan = await self.planner.plan(question, coll)
        logger.info("Plan: %s", plan.summary())

        # 鐘舵€佽拷韪?/ State tracking
        all_results: List[Dict[str, Any]] = []
        all_queries: List[str] = []
        all_seen_ids = set()
        quality_score = 0.0

        # Step 2: 鎵ц鎼滅储璁″垝 / Execute initial search plan
        current_queries = [step.query for step in plan.steps]

        # Step 3-5: 澶氳疆寰幆 / Multi-round loop
        for round_num in range(1, self.config.max_rounds + 1):
            logger.info("=== Round %d/%d ===", round_num, self.config.max_rounds)

            # --- 鎵ц鎼滅储 / Execute searches ---
            round_results = []
            for query in current_queries:
                if query in all_queries:
                    continue  # 璺宠繃宸插皾璇曠殑鏌ヨ / Skip duplicate queries
                all_queries.append(query)

                results = await self._execute_search(query, coll)
                # 鍘婚噸 / Deduplicate
                for r in results:
                    if r["id"] not in all_seen_ids:
                        all_seen_ids.add(r["id"])
                        round_results.append(r)
                        all_results.append(r)

            if not round_results:
                logger.info("No new results in round %d", round_num)
                break

            # --- 璐ㄩ噺璇勪及 / Evaluate quality ---
            eval_result = await self.evaluator.evaluate(
                question, all_results, round_num, self.config.max_rounds
            )
            quality_score = eval_result.score
            logger.info("Quality: %s", eval_result)

            # 璐ㄩ噺杈炬爣鍒欏仠姝?/ Stop if quality is sufficient
            if not eval_result.should_continue:
                logger.info("Quality threshold met, stopping")
                break

            # 杈惧埌鏈€澶ц疆娆″垯鍋滄 / Stop if max rounds reached
            if round_num >= self.config.max_rounds:
                logger.info("Max rounds reached")
                break

            # --- 鏌ヨ閲嶆瀯 / Reformulate queries for next round ---
            new_queries = await self.reformulator.reformulate(
                question,
                all_queries,
                eval_result.feedback,
                strategy_hint=plan.strategy.value,
            )
            current_queries = new_queries
            logger.info("New queries for next round: %s", new_queries)

        # Step 6: 鐢熸垚鏈€缁堢瓟妗?/ Generate final answer
        answer = await self._generate_answer(question, all_results)

        return RetrievalResult(
            answer=answer,
            documents=all_results[: self.config.top_k_final],
            plan=plan,
            rounds=min(round_num, self.config.max_rounds),
            quality_score=quality_score,
            all_queries_tried=all_queries,
        )

    async def _execute_search(
        self, query: str, collection: str
    ) -> List[Dict[str, Any]]:
        """
        鎵ц鍗曟鎼滅储 / Execute a single search against DeepVector.

        娴佺▼: 宓屽叆鏌ヨ鏂囨湰 鈫?POST /search 鈫?杩斿洖缁撴灉

        Args:
            query: 鎼滅储鏌ヨ鏂囨湰
            collection: 闆嗗悎鍚嶇О

        Returns:
            鎼滅储缁撴灉鍒楄〃, 姣忔潯鍖呭惈 id/distance/text/tags/timestamp
        """
        url = f"{self.config.lumendb_url}/search"
        payload = {
            "vector": await self._embed_query(query),
            "k": self.config.max_results_per_round,
        }

        try:
            resp = await self._client.post(url, json=payload)
            resp.raise_for_status()
            data = resp.json()

            results = []
            for item in data.get("results", []):
                meta = await self._fetch_meta(item["id"], collection)
                results.append(
                    {
                        "id": item["id"],
                        "distance": item["distance"],
                        "text": meta.get("text", ""),
                        "tags": meta.get("tags", ""),
                        "timestamp": meta.get("timestamp", 0),
                    }
                )
            return results

        except Exception as e:
            logger.error("Search failed for query '%s': %s", query, e)
            return []

    async def _embed_query(self, text: str) -> List[float]:
        """
        宓屽叆鏌ヨ鏂囨湰 / Embed query text.

        鍒涘缓涓存椂 EmbeddingService 瀹炰緥杩涜宓屽叆銆?
        Args:
            text: 瑕佸祵鍏ョ殑鏂囨湰

        Returns:
            宓屽叆鍚戦噺
        """
        from ..embedding.service import EmbeddingService

        embedder = EmbeddingService(self.config.embedding)
        try:
            return await embedder.embed_single(text)
        finally:
            await embedder.close()

    async def _fetch_meta(
        self, vector_id: int, collection: str
    ) -> Dict[str, Any]:
        """
        鑾峰彇鍚戦噺鐨勫厓鏁版嵁 / Fetch metadata for a vector.

        娉ㄦ剰: 褰撳墠 DeepVector HTTP API 鏆傛湭鎻愪緵閫氳繃 id 鑾峰彇鍏冩暟鎹殑绔偣銆?        姝ゆ柟娉曚负鍗犱綅瀹炵幇, 鍚庣画闇€瑕侀€氳繃 GET /vectors/:id/meta 澧炲己銆?
        Args:
            vector_id: 鍚戦噺 ID
            collection: 闆嗗悎鍚嶇О

        Returns:
            鍏冩暟鎹瓧鍏?        """
        return {"text": "", "tags": "", "timestamp": 0}

    async def _generate_answer(
        self, question: str, results: List[Dict[str, Any]]
    ) -> str:
        """
        鍩轰簬妫€绱㈢粨鏋滅敓鎴愮瓟妗?/ Generate answer from retrieved results.

        灏嗘绱㈠埌鐨勬枃妗ｆ牸寮忓寲涓轰笂涓嬫枃, 鍙戦€佺粰 LLM 鐢熸垚鍥炵瓟銆?
        Args:
            question: 鐢ㄦ埛鍘熷闂
            results: 鎵€鏈夋绱㈠埌鐨勭粨鏋?
        Returns:
            LLM 鐢熸垚鐨勭瓟妗堟枃鏈?        """
        if not results:
            return "No relevant documents found."

        # 鏍煎紡鍖栨枃妗ｆ憳瑕?/ Format document summaries
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
        """鍏抽棴寮曟搸, 閲婃斁璧勬簮 / Close engine and release resources."""
        if self._client and not self._client.is_closed:
            await self._client.aclose()
