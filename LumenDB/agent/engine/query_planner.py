"""
查询规划器 / Query Planner — 自然语言问题 → 检索策略。

核心流程 / Core Flow:
  1. 接收用户自然语言问题
  2. 调用 LLM 分析问题类型
  3. LLM 选择策略 (DIRECT/FILTERED/MULTI_QUERY/HIERARCHICAL)
  4. 生成结构化的 RetrievalPlan

架构注意 / Architecture Note:
  查询规划是 AgenticDB 区别于普通向量数据库的核心特性。
  传统向量数据库需要开发者手动编码搜索逻辑,
  AgenticDB 让 LLM 自动完成这个决策过程。
"""

import json
import logging
from typing import Any, Dict, Optional

from ..llm.router import LLMRouter
from ..llm.prompts import QUERY_PLANNER_SYSTEM
from ..llm.schemas import ALL_TOOLS
from .strategy import RetrievalPlan, SearchStep, SearchStrategy

logger = logging.getLogger(__name__)


class QueryPlanner:
    """
    查询规划器 / Query Planner.

    分析用户的自然语言问题, 生成结构化的检索计划。
    使用 LLM 的语义理解能力和工具调用能力。

    用法 / Usage:
        planner = QueryPlanner(llm)
        plan = await planner.plan("What is RAG?")
        print(plan.strategy)  # SearchStrategy.DIRECT
    """

    def __init__(self, llm: LLMRouter, default_collection: str = "default"):
        """
        初始化查询规划器 / Initialize query planner.

        Args:
            llm: LLM 路由器实例
            default_collection: 默认集合名称
        """
        self.llm = llm
        self.default_collection = default_collection

    async def plan(
        self, question: str, collection: Optional[str] = None
    ) -> RetrievalPlan:
        """
        分析问题并生成检索计划 / Analyze question and generate retrieval plan.

        步骤:
          1. 将问题和 system prompt 一起发送给 LLM
          2. 提供工具定义让 LLM 可以选择调用
          3. 优先尝试从 tool_call 提取计划 (更可靠)
          4. 回退到解析 JSON content

        Args:
            question: 用户的自然语言问题
            collection: 集合名称 (可选)

        Returns:
            RetrievalPlan 包含策略、步骤和预期轮数
        """
        coll = collection or self.default_collection

        messages = [
            {"role": "system", "content": QUERY_PLANNER_SYSTEM},
            {
                "role": "user",
                "content": f"Question: {question}\nCollection: {coll}\n\nGenerate a retrieval plan.",
            },
        ]

        response = await self.llm.chat(messages, tools=ALL_TOOLS)

        # 策略 1: 优先从 tool_call 提取 / Priority: extract from tool calls
        if response.tool_calls:
            for tc in response.tool_calls:
                if tc["name"] == "vector_search":
                    return self._plan_from_single_search(
                        tc["arguments"], question, coll
                    )
                elif tc["name"] == "filtered_search":
                    return self._plan_from_filtered_search(
                        tc["arguments"], question, coll
                    )

        # 策略 2: 从 JSON content 解析 / Fallback: parse JSON from content
        return self._plan_from_json(response.content, question, coll)

    def _plan_from_single_search(
        self, args: Dict[str, Any], question: str, collection: str
    ) -> RetrievalPlan:
        """
        从 vector_search 工具调用生成计划 / Extract plan from vector_search tool call.

        当 LLM 直接选择了 vector_search 工具时,
        我们将其参数映射为 DIRECT 策略的 RetrievalPlan。

        Args:
            args: vector_search 的参数 (query, k, collection)
            question: 原始用户问题
            collection: 集合名称

        Returns:
            DIRECT 策略的检索计划
        """
        return RetrievalPlan(
            strategy=SearchStrategy.DIRECT,
            reasoning="LLM chose direct search via tool call",
            steps=[
                SearchStep(
                    query=args.get("query", question),
                    k=args.get("k", 10),
                    collection=args.get("collection", collection),
                )
            ],
            expected_rounds=1,
            original_question=question,
        )

    def _plan_from_filtered_search(
        self, args: Dict[str, Any], question: str, collection: str
    ) -> RetrievalPlan:
        """
        从 filtered_search 工具调用生成计划 / Extract plan from filtered_search call.

        Args:
            args: filtered_search 的参数 (query, filter, k, collection)
            question: 原始用户问题
            collection: 集合名称

        Returns:
            FILTERED 策略的检索计划
        """
        return RetrievalPlan(
            strategy=SearchStrategy.FILTERED,
            reasoning="LLM chose filtered search via tool call",
            steps=[
                SearchStep(
                    query=args.get("query", question),
                    filter=args.get("filter"),
                    k=args.get("k", 10),
                    collection=args.get("collection", collection),
                )
            ],
            expected_rounds=1,
            original_question=question,
        )

    def _plan_from_json(
        self, content: str, question: str, collection: str
    ) -> RetrievalPlan:
        """
        从 JSON 文本解析计划 / Parse plan from JSON text.

        当 LLM 没有选择 tool_call 时, 我们期望它返回了 JSON 格式的计划。
        需要处理 markdown 代码块包装和 JSON 解析。

        Args:
            content: LLM 返回的文本 (应包含 JSON)
            question: 原始用户问题
            collection: 集合名称

        Returns:
            解析后的 RetrievalPlan, 解析失败则回退到 DIRECT
        """
        try:
            # 去除可能存在的 markdown 代码块围栏 / Strip markdown fences
            text = content.strip()
            if "```" in text:
                text = text.split("```")[1]
                if text.startswith("json"):
                    text = text[4:]
                text = text.strip()

            plan_data = json.loads(text)
            strategy_str = plan_data.get("strategy", "DIRECT").upper()

            # 字符串到枚举的映射 / String to enum mapping
            strategy_map = {
                "DIRECT": SearchStrategy.DIRECT,
                "FILTERED": SearchStrategy.FILTERED,
                "MULTI_QUERY": SearchStrategy.MULTI_QUERY,
                "HIERARCHICAL": SearchStrategy.HIERARCHICAL,
            }
            strategy = strategy_map.get(strategy_str, SearchStrategy.DIRECT)

            # 解析搜索步骤 / Parse search steps
            steps = []
            for s in plan_data.get("searches", []):
                steps.append(
                    SearchStep(
                        query=s.get("query", question),
                        filter=s.get("filter"),
                        k=s.get("k", 10),
                        collection=s.get("collection", collection),
                    )
                )

            if not steps:
                steps = [SearchStep(query=question, k=10, collection=collection)]

            return RetrievalPlan(
                strategy=strategy,
                reasoning=plan_data.get("reasoning", "LLM generated plan"),
                steps=steps,
                expected_rounds=plan_data.get("expected_rounds", 1),
                original_question=question,
            )

        except (json.JSONDecodeError, KeyError, ValueError) as e:
            logger.warning(
                "Failed to parse plan JSON, falling back to DIRECT: %s", e
            )
            return self._fallback_plan(question, collection)

    def _fallback_plan(self, question: str, collection: str) -> RetrievalPlan:
        """
        回退计划 / Fallback plan.

        当 LLM 返回内容无法解析时使用。
        保证系统在任何情况下都能正常工作。

        Args:
            question: 用户问题
            collection: 集合名称

        Returns:
            最简单的 DIRECT 策略计划
        """
        return RetrievalPlan(
            strategy=SearchStrategy.DIRECT,
            reasoning="Fallback: LLM response parsing failed",
            steps=[SearchStep(query=question, k=10, collection=collection)],
            expected_rounds=1,
            original_question=question,
        )
