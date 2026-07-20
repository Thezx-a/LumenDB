"""
缁撴灉璇勪及鍣?/ Result Evaluator 鈥?鑷姩鍒ゆ柇妫€绱㈢粨鏋滄槸鍚﹁冻澶熴€?
杩欐槸 AgenticDB 鑷瘎浼拌兘鍔涚殑鏍稿績缁勪欢銆?涓庝紶缁?RAG 绯荤粺涓嶅悓 (鎬绘槸杩斿洖 top-k 缁撴灉),
AgenticDB 浼氳瘎浼扮粨鏋滆川閲忓苟鍐冲畾鏄惁闇€瑕佹洿澶氭绱€?
璇勪及缁村害 / Evaluation Dimensions:
  1. Relevance (鐩稿叧鎬?: 缁撴灉鏄惁鐪熸涓庨棶棰樼浉鍏?  2. Coverage (瑕嗙洊搴?: 鏄惁瑕嗙洊浜嗛棶棰樼殑鍚勪釜鏂归潰
  3. Sufficiency (鍏呭垎鎬?: 淇℃伅鏄惁瓒冲褰㈡垚濂界殑绛旀

鍐崇瓥閫昏緫 / Decision Logic:
  - score < quality_threshold (0.7): 缁х画妫€绱?  - score 鈮?quality_threshold: 鍋滄妫€绱? 鐢熸垚绛旀
"""

import json
import logging
from typing import Any, Dict, List, Optional

from ..llm.router import LLMRouter, LLMResponse
from ..llm.prompts import RESULT_EVALUATOR_SYSTEM

logger = logging.getLogger(__name__)


class EvalResult:
    """
    璇勪及缁撴灉 / Evaluation Result.

    鍖呭惈璇勫垎銆佸悇缁村害寰楀垎銆佹枃鏈弽棣堝拰鏄惁缁х画鐨勫缓璁€?
    Attributes:
        score: 缁煎悎璐ㄩ噺璇勫垎 (0-1)
        relevance: 鐩稿叧鎬ц瘎鍒?(0-1)
        coverage: 瑕嗙洊搴﹁瘎鍒?(0-1)
        sufficiency: 鍏呭垎鎬ц瘎鍒?(0-1)
        feedback: 鏂囨湰鍙嶉, 璇存槑鏀硅繘鏂瑰悜
        should_continue: 鏄惁寤鸿缁х画妫€绱?    """

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
    缁撴灉璐ㄩ噺璇勪及鍣?/ Result Quality Evaluator.

    浣跨敤 LLM 璇勪及妫€绱㈢粨鏋滅殑璐ㄩ噺, 鍐冲畾鏄惁闇€瑕佺户缁绱€?
    鐢ㄦ硶 / Usage:
        evaluator = ResultEvaluator(llm, quality_threshold=0.7)
        result = await evaluator.evaluate("What is RAG?", retrieved_docs)

        if result.should_continue:
            print(f"Need more: {result.feedback}")
        else:
            print(f"Quality: {result.score:.2f}, ready to answer")
    """

    def __init__(self, llm: LLMRouter, quality_threshold: float = 0.7):
        """
        鍒濆鍖栫粨鏋滆瘎浼板櫒 / Initialize result evaluator.

        Args:
            llm: LLM 璺敱鍣ㄥ疄渚? 鐢ㄤ簬璋冪敤璇勪及妯″瀷
            quality_threshold: 璐ㄩ噺闃堝€? 杈惧埌姝ゅ垎鏁板嵆璁や负瓒冲
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
        璇勪及妫€绱㈢粨鏋滆川閲?/ Evaluate search result quality.

        灏嗛棶棰樺拰妫€绱㈢粨鏋滄牸寮忓寲鍚庡彂閫佺粰 LLM 璇勪及銆?        LLM 杩斿洖 JSON 鏍煎紡鐨勮瘎鍒嗗拰鍙嶉銆?
        Args:
            question: 鐢ㄦ埛鍘熷闂
            results: 褰撳墠杞妫€绱㈠埌鐨勬柊缁撴灉鍒楄〃
            round_num: 褰撳墠杞 (浠?寮€濮?
            max_rounds: 鏈€澶ц疆娆￠檺鍒?
        Returns:
            EvalResult 璇勪及缁撴灉瀵硅薄
        """
        if not results:
            return EvalResult(
                score=0.0,
                feedback="No results retrieved",
                should_continue=True,
            )

        # 鏍煎紡鍖栫粨鏋滀负 LLM 鍙鐨勬枃鏈?        results_text = self._format_results(results)

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
        灏嗘绱㈢粨鏋滄牸寮忓寲涓?LLM 鍙鐨勬枃鏈?/ Format results for LLM.

        姣忔潯缁撴灉鍖呭惈 id銆乨istance 鍜岄儴鍒嗘枃鏈唴瀹广€?        鏂囨湰鎴柇鍒板墠 100 瀛楃浠ヨ妭鐪?token銆?
        Args:
            results: 妫€绱㈢粨鏋滃垪琛?
        Returns:
            鏍煎紡鍖栧悗鐨勬枃鏈瓧绗︿覆
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
        瑙ｆ瀽 LLM 杩斿洖鐨勮瘎浼扮粨鏋?JSON / Parse LLM evaluation response.

        闇€瑕佸鐞?
          - 绾?JSON 鏂囨湰
          - markdown 浠ｇ爜鍧楀寘瑁呯殑 JSON
          - 闈?JSON 鍐呭 (瑙ｆ瀽澶辫触鏃惰繑鍥為粯璁ゅ€?

        Args:
            response: LLM 鍝嶅簲瀵硅薄

        Returns:
            瑙ｆ瀽鍚庣殑 EvalResult
        """
        try:
            text = response.content.strip()
            # 鍘婚櫎 markdown 浠ｇ爜鍧?/ Strip markdown fences
            if "```" in text:
                text = text.split("```")[1]
                if text.startswith("json"):
                    text = text[4:]
                text = text.strip()

            data = json.loads(text)

            score = float(data.get("score", 0.5))

            # 鍐冲畾鏄惁缁х画: 浣庝簬闃堝€煎垯缁х画
            should_continue = score < self.quality_threshold

            # 浣嗗鏋?LLM 鏄庣‘璇?should_continue=False, 鍗充娇鍒嗘暟浣庝篃鍋滄
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
            # 瑙ｆ瀽澶辫触鏃朵繚瀹堝鐞? 鍋滄妫€绱?            return EvalResult(
                score=0.5,
                feedback=f"Evaluation parsing failed: {e}",
                should_continue=False,
            )
