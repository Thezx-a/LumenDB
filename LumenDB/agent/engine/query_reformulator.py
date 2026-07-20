"""
查询重构器 / Query Reformulator — 生成改进的搜索查询。

当初始检索结果不足时 (由 ResultEvaluator 判断),
QueryReformulator 使用 LLM 生成更好的查询。

改进策略 / Improvement Strategies:
  1. 同义词/近义词替换 / Synonym substitution
  2. 添加领域术语 / Add domain-specific terms
  3. 改变查询粒度 / Change query granularity
  4. 切换查询角度 / Switch query perspective
  5. 分解复杂概念 / Decompose complex concepts

架构注意 / Architecture Note:
  查询重构是多轮检索的核心机制。
  它模拟了人类搜索时的"换关键词/换角度"行为,
  是 AgenticDB 区别于单轮检索系统的关键差异点。
"""

import json
import logging
from typing import Any, Dict, List, Optional

from ..llm.router import LLMRouter, LLMResponse
from ..llm.prompts import QUERY_REFORMULATOR_SYSTEM

logger = logging.getLogger(__name__)


class QueryReformulator:
    """
    查询重构器 / Query Reformulator.

    根据评估反馈生成改进的搜索查询。

    用法 / Usage:
        reformulator = QueryReformulator(llm)
        new_queries = await reformulator.reformulate(
            question="How does HNSW work?",
            previous_queries=["HNSW explanation"],
            feedback="Results are too general, need more technical depth"
        )
        print(new_queries)  # ["HNSW graph structure nearest neighbor", ...]
    """

    def __init__(self, llm: LLMRouter):
        """
        初始化查询重构器 / Initialize query reformulator.

        Args:
            llm: LLM 路由器实例
        """
        self.llm = llm

    async def reformulate(
        self,
        question: str,
        previous_queries: List[str],
        feedback: str,
        strategy_hint: Optional[str] = None,
    ) -> List[str]:
        """
        生成改进的搜索查询 / Generate improved search queries.

        将原始问题、已尝试的查询和反馈一起发送给 LLM,
        LLM 返回一组新的查询字符串。

        Args:
            question: 用户的原始问题
            previous_queries: 本轮之前已尝试过的查询列表
            feedback: ResultEvaluator 给出的具体反馈
            strategy_hint: 策略提示 (如 "direct", "multi_query"),
                         帮助 LLM 生成更合适的查询

        Returns:
            新的搜索查询列表

        Note:
            如果 LLM 返回空列表或解析失败, 使用回退机制返回原始问题。
        """
        messages = [
            {"role": "system", "content": QUERY_REFORMULATOR_SYSTEM},
            {
                "role": "user",
                "content": self._build_prompt(
                    question, previous_queries, feedback, strategy_hint
                ),
            },
        ]

        response = await self.llm.chat(messages)
        return self._parse_queries(response, question)

    def _build_prompt(
        self,
        question: str,
        previous_queries: List[str],
        feedback: str,
        strategy_hint: Optional[str],
    ) -> str:
        """
        构建查询重构的 prompt / Build reformulation prompt.

        包含原始问题、已尝试的查询和评估反馈,
        让 LLM 充分理解上下文后生成改进查询。

        Args:
            question: 原始用户问题
            previous_queries: 已尝试的查询列表
            feedback: 评估反馈
            strategy_hint: 策略提示 (可选)

        Returns:
            prompt 字符串
        """
        parts = [f"Original question: {question}"]

        if previous_queries:
            parts.append("\nQueries already tried:")
            for i, q in enumerate(previous_queries):
                parts.append(f"  {i+1}. {q}")

        parts.append(f"\nFeedback on current results:\n{feedback}")

        if strategy_hint:
            parts.append(f"\nStrategy hint: {strategy_hint}")

        parts.append("\nGenerate new, improved search queries.")
        return "\n".join(parts)

    def _parse_queries(
        self, response: LLMResponse, question: str
    ) -> List[str]:
        """
        解析 LLM 返回的查询重构结果 / Parse reformulation response.

        需要处理:
          - 纯 JSON 文本
          - markdown 代码块包装的 JSON

        Args:
            response: LLM 响应对象
            question: 原始问题 (用于回退)

        Returns:
            查询字符串列表, 解析失败则返回 [question]
        """
        try:
            text = response.content.strip()
            if "```" in text:
                text = text.split("```")[1]
                if text.startswith("json"):
                    text = text[4:]
                text = text.strip()

            data = json.loads(text)
            queries = data.get("new_queries", [])

            if not queries:
                logger.warning("LLM returned empty queries, using fallback")
                return self._fallback_queries(question)

            return queries

        except (json.JSONDecodeError, ValueError) as e:
            logger.warning(
                "Failed to parse reformulation JSON: %s", e
            )
            return self._fallback_queries(question)

    def _fallback_queries(self, question: str) -> List[str]:
        """
        回退策略: 返回原始问题作为查询 / Fallback: use original question.

        当 LLM 解析失败时, 至少返回原始查询,
        保证系统在异常情况下仍能工作。

        Args:
            question: 原始用户问题

        Returns:
            包含原始问题的单元素列表
        """
        return [question]
