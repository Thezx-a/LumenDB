"""
多轮检索引擎 / Multi-Round Retrieval Engine — AgenticDB 的核心编排器。

核心流程 / Core Pipeline:
  1. QueryPlanner → RetrievalPlan (策略选择)
  2. 执行搜索步骤 (嵌入 + DeepVector 搜索)
  3. ResultEvaluator → 质量评分
  4. 如果质量不足: QueryReformulator → 新查询 → 回到步骤2
  5. 如果质量达标: AnswerGenerator → 最终答案

数据流 / Data Flow:
  User Question
    ↓
  [QueryPlanner]
    ↓ RetrievalPlan
  [MultiRoundEngine.Round 1]
    ├── embed question
    ├── search DeepVector
    └── collect results
    ↓
  [ResultEvaluator] ← LLM
    ├── score ≥ 0.7 → [AnswerGenerator] → Final Answer
    └── score < 0.7
         ↓
  [QueryReformulator] ← LLM
    ↓ new_queries
  [MultiRoundEngine.Round 2] ...循环

设计原则 / Design Principles:
  - 异步全链路: 所有 IO 都是 async (LLM 调用 + 数据库查询)
  - 容错设计: 每轮搜索失败不阻断整体流程
  - 质量驾驶: 结果质量决定是否继续检索, 而非固定轮数
  - 资源保护: max_rounds 防止无限循环
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
    完整检索结果 / Complete retrieval result.

    包含最终答案、检索到的文档、使用的计划、轮次信息和质量评分。
    所有字段可序列化为 JSON 用于 API 响应。

    Attributes:
        answer: LLM 生成的最终答案文本
        documents: 检索到的文档列表
        plan: 使用的检索计划
        rounds: 实际执行的轮次数
        quality_score: 最终质量评分
        all_queries_tried: 所有尝试过的查询
    """

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
        序列化为字典 / Serialize to dictionary.

        用于 JSON 序列化 (API 响应和日志)。

        Returns:
            包含所有字段的字典
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
    多轮检索引擎 / Multi-Round Retrieval Engine.

    编排完整的检索流程: 规划 → 执行 → 评估 → 重构(如有需要) → 回答。

    这是 AgenticDB 的核心类, 串联了所有组件。

    用法 / Usage:
        engine = MultiRoundEngine(config, llm)
        result = await engine.retrieve("What is RAG and how does it work?")
        print(result.answer)
        print(f"Strategy: {result.plan.strategy.value}")
        print(f"Rounds: {result.rounds}")
    """

    def __init__(self, config: AgenticDBConfig, llm: LLMRouter):
        """
        初始化多轮检索引擎 / Initialize multi-round engine.

        Args:
            config: AgenticDB 全局配置
            llm: LLM 路由器实例
        """
        self.config = config
        self.llm = llm
        self.planner = QueryPlanner(llm, config.default_collection)
        self.evaluator = ResultEvaluator(llm, config.quality_threshold)
        self.reformulator = QueryReformulator(llm)
        self._client: Optional[httpx.AsyncClient] = None

    async def _ensure_client(self):
        """确保 DeepVector HTTP 客户端已初始化 / Ensure HTTP client initialized."""
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    async def retrieve(
        self, question: str, collection: Optional[str] = None
    ) -> RetrievalResult:
        """
        执行多轮检索 / Execute multi-round retrieval.

        完整流程:
          1. QueryPlanner 生成检索计划
          2. 执行第一轮搜索 (按计划的 steps)
          3. ResultEvaluator 评估结果质量
          4. 质量不足 → QueryReformulator → 执行新轮次
          5. 质量达标或超轮次 → 生成最终答案

        Args:
            question: 用户的自然语言问题
            collection: 集合名称 (可选)

        Returns:
            RetrievalResult 包含答案、文档和元信息
        """
        await self._ensure_client()
        coll = collection or self.config.default_collection

        # Step 1: 查询规划 / Generate retrieval plan
        plan = await self.planner.plan(question, coll)
        logger.info("Plan: %s", plan.summary())

        # 状态追踪 / State tracking
        all_results: List[Dict[str, Any]] = []
        all_queries: List[str] = []
        all_seen_ids = set()
        quality_score = 0.0

        # Step 2: 执行搜索计划 / Execute initial search plan
        current_queries = [step.query for step in plan.steps]

        # Step 3-5: 多轮循环 / Multi-round loop
        for round_num in range(1, self.config.max_rounds + 1):
            logger.info("=== Round %d/%d ===", round_num, self.config.max_rounds)

            # --- 执行搜索 / Execute searches ---
            round_results = []
            for query in current_queries:
                if query in all_queries:
                    continue  # 跳过已尝试的查询 / Skip duplicate queries
                all_queries.append(query)

                results = await self._execute_search(query, coll)
                # 去重 / Deduplicate
                for r in results:
                    if r["id"] not in all_seen_ids:
                        all_seen_ids.add(r["id"])
                        round_results.append(r)
                        all_results.append(r)

            if not round_results:
                logger.info("No new results in round %d", round_num)
                break

            # --- 质量评估 / Evaluate quality ---
            eval_result = await self.evaluator.evaluate(
                question, all_results, round_num, self.config.max_rounds
            )
            quality_score = eval_result.score
            logger.info("Quality: %s", eval_result)

            # 质量达标则停止 / Stop if quality is sufficient
            if not eval_result.should_continue:
                logger.info("Quality threshold met, stopping")
                break

            # 达到最大轮次则停止 / Stop if max rounds reached
            if round_num >= self.config.max_rounds:
                logger.info("Max rounds reached")
                break

            # --- 查询重构 / Reformulate queries for next round ---
            new_queries = await self.reformulator.reformulate(
                question,
                all_queries,
                eval_result.feedback,
                strategy_hint=plan.strategy.value,
            )
            current_queries = new_queries
            logger.info("New queries for next round: %s", new_queries)

        # Step 6: 生成最终答案 / Generate final answer
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
        执行单次搜索 / Execute a single search against DeepVector.

        流程: 嵌入查询文本 → POST /search → 返回结果

        Args:
            query: 搜索查询文本
            collection: 集合名称

        Returns:
            搜索结果列表, 每条包含 id/distance/text/tags/timestamp
        """
        url = f"{self.config.deepvector_url}/search"
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
        嵌入查询文本 / Embed query text.

        创建临时 EmbeddingService 实例进行嵌入。

        Args:
            text: 要嵌入的文本

        Returns:
            嵌入向量
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
        获取向量的元数据 / Fetch metadata for a vector.

        注意: 当前 DeepVector HTTP API 暂未提供通过 id 获取元数据的端点。
        此方法为占位实现, 后续需要通过 GET /vectors/:id/meta 增强。

        Args:
            vector_id: 向量 ID
            collection: 集合名称

        Returns:
            元数据字典
        """
        return {"text": "", "tags": "", "timestamp": 0}

    async def _generate_answer(
        self, question: str, results: List[Dict[str, Any]]
    ) -> str:
        """
        基于检索结果生成答案 / Generate answer from retrieved results.

        将检索到的文档格式化为上下文, 发送给 LLM 生成回答。

        Args:
            question: 用户原始问题
            results: 所有检索到的结果

        Returns:
            LLM 生成的答案文本
        """
        if not results:
            return "No relevant documents found."

        # 格式化文档摘要 / Format document summaries
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
        """关闭引擎, 释放资源 / Close engine and release resources."""
        if self._client and not self._client.is_closed:
            await self._client.aclose()
