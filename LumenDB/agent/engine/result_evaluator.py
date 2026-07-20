"""
结果评估器 / Result Evaluator — 自动判断检索结果是否足够。

这是 AgenticDB 自评估能力的核心组件。
与传统 RAG 系统不同 (总是返回 top-k 结果),
AgenticDB 会评估结果质量并决定是否需要更多检索。

评估维度 / Evaluation Dimensions:
  1. Relevance (相关性): 结果是否真正与问题相关
  2. Coverage (覆盖度): 是否覆盖了问题的各个方面
  3. Sufficiency (充分性): 信息是否足够形成好的答案

决策逻辑 / Decision Logic:
  - score < quality_threshold (0.7): 继续检索
  - score ≥ quality_threshold: 停止检索, 生成答案
"""

import json
import logging
from typing import Any, Dict, List, Optional

from ..llm.router import LLMRouter, LLMResponse
from ..llm.prompts import RESULT_EVALUATOR_SYSTEM

logger = logging.getLogger(__name__)


class EvalResult:
    """
    评估结果 / Evaluation Result.

    包含评分、各维度得分、文本反馈和是否继续的建议。

    Attributes:
        score: 综合质量评分 (0-1)
        relevance: 相关性评分 (0-1)
        coverage: 覆盖度评分 (0-1)
        sufficiency: 充分性评分 (0-1)
        feedback: 文本反馈, 说明改进方向
        should_continue: 是否建议继续检索
    """

    def __init__(
        self,
        score: float = 0.0,
        relevance: float = 0.0,
        coverage: float = 0.0,
        sufficiency: float = 0.0,
        feedback: str = "",
        should_continue: bool = True,
    ):
        self.score = score
        self.relevance = relevance
        self.coverage = coverage
        self.sufficiency = sufficiency
        self.feedback = feedback
        self.should_continue = should_continue

    def __repr__(self):
        return (
            f"EvalResult(score={self.score:.2f}, rel={self.relevance:.2f}, "
            f"cov={self.coverage:.2f}, suf={self.sufficiency:.2f}, "
            f"continue={self.should_continue})"
        )


class ResultEvaluator:
    """
    结果质量评估器 / Result Quality Evaluator.

    使用 LLM 评估检索结果的质量, 决定是否需要继续检索。

    用法 / Usage:
        evaluator = ResultEvaluator(llm, quality_threshold=0.7)
        result = await evaluator.evaluate("What is RAG?", retrieved_docs)

        if result.should_continue:
            print(f"Need more: {result.feedback}")
        else:
            print(f"Quality: {result.score:.2f}, ready to answer")
    """

    def __init__(self, llm: LLMRouter, quality_threshold: float = 0.7):
        """
        初始化结果评估器 / Initialize result evaluator.

        Args:
            llm: LLM 路由器实例, 用于调用评估模型
            quality_threshold: 质量阈值, 达到此分数即认为足够
        """
        self.llm = llm
        self.quality_threshold = quality_threshold

    async def evaluate(
        self,
        question: str,
        results: List[Dict[str, Any]],
        round_num: int = 1,
        max_rounds: int = 5,
    ) -> EvalResult:
        """
        评估检索结果质量 / Evaluate search result quality.

        将问题和检索结果格式化后发送给 LLM 评估。
        LLM 返回 JSON 格式的评分和反馈。

        Args:
            question: 用户原始问题
            results: 当前轮次检索到的新结果列表
            round_num: 当前轮次 (从1开始)
            max_rounds: 最大轮次限制

        Returns:
            EvalResult 评估结果对象
        """
        if not results:
            return EvalResult(
                score=0.0,
                feedback="No results retrieved",
                should_continue=True,
            )

        # 格式化结果为 LLM 可读的文本
        results_text = self._format_results(results)

        messages = [
            {"role": "system", "content": RESULT_EVALUATOR_SYSTEM},
            {
                "role": "user",
                "content": (
                    f"Original question: {question}\n\n"
                    f"Retrieved results (round {round_num}/{max_rounds}):\n"
                    f"{results_text}\n\n"
                    f"Evaluate the quality of these results."
                ),
            },
        ]

        response = await self.llm.chat(messages)
        return self._parse_evaluation(response)

    def _format_results(self, results: List[Dict[str, Any]]) -> str:
        """
        将检索结果格式化为 LLM 可读的文本 / Format results for LLM.

        每条结果包含 id、distance 和部分文本内容。
        文本截断到前 100 字符以节省 token。

        Args:
            results: 检索结果列表

        Returns:
            格式化后的文本字符串
        """
        parts = []
        for i, r in enumerate(results):
            meta_text = ""
            if r.get("text"):
                meta_text = f" | {r['text'][:100]}..."
            parts.append(
                f"[Doc {i+1}] id={r.get('id', '?')} "
                f"distance={r.get('distance', 0):.4f}{meta_text}"
            )
        return "\n".join(parts)

    def _parse_evaluation(self, response: LLMResponse) -> EvalResult:
        """
        解析 LLM 返回的评估结果 JSON / Parse LLM evaluation response.

        需要处理:
          - 纯 JSON 文本
          - markdown 代码块包装的 JSON
          - 非 JSON 内容 (解析失败时返回默认值)

        Args:
            response: LLM 响应对象

        Returns:
            解析后的 EvalResult
        """
        try:
            text = response.content.strip()
            # 去除 markdown 代码块 / Strip markdown fences
            if "```" in text:
                text = text.split("```")[1]
                if text.startswith("json"):
                    text = text[4:]
                text = text.strip()

            data = json.loads(text)

            score = float(data.get("score", 0.5))

            # 决定是否继续: 低于阈值则继续
            should_continue = score < self.quality_threshold

            # 但如果 LLM 明确说 should_continue=False, 即使分数低也停止
            if not data.get("should_continue", True):
                should_continue = False

            return EvalResult(
                score=score,
                relevance=float(data.get("relevance", 0.5)),
                coverage=float(data.get("coverage", 0.5)),
                sufficiency=float(data.get("sufficiency", 0.5)),
                feedback=data.get("feedback", ""),
                should_continue=should_continue,
            )

        except (json.JSONDecodeError, ValueError) as e:
            logger.warning(
                "Failed to parse evaluation JSON: %s", e
            )
            # 解析失败时保守处理: 停止检索
            return EvalResult(
                score=0.5,
                feedback=f"Evaluation parsing failed: {e}",
                should_continue=False,
            )
