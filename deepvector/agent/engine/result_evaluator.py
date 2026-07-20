"""
Result Evaluator — assess whether retrieval results are sufficient.

Uses an LLM to score relevance, coverage, and sufficiency, then decides
whether to continue multi-round retrieval.
"""

import json
import logging
from typing import Any, Dict, List

from ..llm.router import LLMRouter, LLMResponse
from ..llm.prompts import RESULT_EVALUATOR_SYSTEM

logger = logging.getLogger(__name__)


class EvalResult:
    """Evaluation result with scores and continuation advice."""

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
    """Evaluate search result quality and decide whether to continue."""

    def __init__(self, llm: LLMRouter, quality_threshold: float = 0.7):
        self.llm = llm
        self.quality_threshold = quality_threshold

    async def evaluate(
        self,
        question: str,
        results: List[Dict[str, Any]],
        round_num: int = 1,
        max_rounds: int = 5,
    ) -> EvalResult:
        if not results:
            return EvalResult(
                score=0.0,
                feedback="No results retrieved",
                should_continue=True,
            )

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
        try:
            text = response.content.strip()
            if "```" in text:
                text = text.split("```")[1]
                if text.startswith("json"):
                    text = text[4:]
                text = text.strip()

            data = json.loads(text)
            score = float(data.get("score", 0.5))
            should_continue = score < self.quality_threshold

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
            logger.warning("Failed to parse evaluation JSON: %s", e)
            return EvalResult(
                score=0.5,
                feedback=f"Evaluation parsing failed: {e}",
                should_continue=False,
            )
