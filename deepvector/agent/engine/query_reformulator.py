"""
鏌ヨ閲嶆瀯鍣?/ Query Reformulator 鈥?鐢熸垚鏀硅繘鐨勬悳绱㈡煡璇€?
褰撳垵濮嬫绱㈢粨鏋滀笉瓒虫椂 (鐢?ResultEvaluator 鍒ゆ柇),
QueryReformulator 浣跨敤 LLM 鐢熸垚鏇村ソ鐨勬煡璇€?
鏀硅繘绛栫暐 / Improvement Strategies:
  1. 鍚屼箟璇?杩戜箟璇嶆浛鎹?/ Synonym substitution
  2. 娣诲姞棰嗗煙鏈 / Add domain-specific terms
  3. 鏀瑰彉鏌ヨ绮掑害 / Change query granularity
  4. 鍒囨崲鏌ヨ瑙掑害 / Switch query perspective
  5. 鍒嗚В澶嶆潅姒傚康 / Decompose complex concepts

鏋舵瀯娉ㄦ剰 / Architecture Note:
  鏌ヨ閲嶆瀯鏄杞绱㈢殑鏍稿績鏈哄埗銆?  瀹冩ā鎷熶簡浜虹被鎼滅储鏃剁殑"鎹㈠叧閿瘝/鎹㈣搴?琛屼负,
  鏄?AgenticDB 鍖哄埆浜庡崟杞绱㈢郴缁熺殑鍏抽敭宸紓鐐广€?"""

import json
import logging
from typing import Any, Dict, List, Optional

from ..llm.router import LLMRouter, LLMResponse
from ..llm.prompts import QUERY_REFORMULATOR_SYSTEM

logger = logging.getLogger(__name__)


class QueryReformulator:
    """
    鏌ヨ閲嶆瀯鍣?/ Query Reformulator.

    鏍规嵁璇勪及鍙嶉鐢熸垚鏀硅繘鐨勬悳绱㈡煡璇€?
    鐢ㄦ硶 / Usage:
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
        鍒濆鍖栨煡璇㈤噸鏋勫櫒 / Initialize query reformulator.

        Args:
            llm: LLM 璺敱鍣ㄥ疄渚?        """
        self.llm = llm

    async def reformulate(
        self,
        question: str,
        previous_queries: List[str],
        feedback: str,
        strategy_hint: Optional[str] = None,
    ) -> List[str]:
        """
        鐢熸垚鏀硅繘鐨勬悳绱㈡煡璇?/ Generate improved search queries.

        灏嗗師濮嬮棶棰樸€佸凡灏濊瘯鐨勬煡璇㈠拰鍙嶉涓€璧峰彂閫佺粰 LLM,
        LLM 杩斿洖涓€缁勬柊鐨勬煡璇㈠瓧绗︿覆銆?
        Args:
            question: 鐢ㄦ埛鐨勫師濮嬮棶棰?            previous_queries: 鏈疆涔嬪墠宸插皾璇曡繃鐨勬煡璇㈠垪琛?            feedback: ResultEvaluator 缁欏嚭鐨勫叿浣撳弽棣?            strategy_hint: 绛栫暐鎻愮ず (濡?"direct", "multi_query"),
                         甯姪 LLM 鐢熸垚鏇村悎閫傜殑鏌ヨ

        Returns:
            鏂扮殑鎼滅储鏌ヨ鍒楄〃

        Note:
            濡傛灉 LLM 杩斿洖绌哄垪琛ㄦ垨瑙ｆ瀽澶辫触, 浣跨敤鍥為€€鏈哄埗杩斿洖鍘熷闂銆?        """
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
        鏋勫缓鏌ヨ閲嶆瀯鐨?prompt / Build reformulation prompt.

        鍖呭惈鍘熷闂銆佸凡灏濊瘯鐨勬煡璇㈠拰璇勪及鍙嶉,
        璁?LLM 鍏呭垎鐞嗚В涓婁笅鏂囧悗鐢熸垚鏀硅繘鏌ヨ銆?
        Args:
            question: 鍘熷鐢ㄦ埛闂
            previous_queries: 宸插皾璇曠殑鏌ヨ鍒楄〃
            feedback: 璇勪及鍙嶉
            strategy_hint: 绛栫暐鎻愮ず (鍙€?

        Returns:
            prompt 瀛楃涓?        """
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
        瑙ｆ瀽 LLM 杩斿洖鐨勬煡璇㈤噸鏋勭粨鏋?/ Parse reformulation response.

        闇€瑕佸鐞?
          - 绾?JSON 鏂囨湰
          - markdown 浠ｇ爜鍧楀寘瑁呯殑 JSON

        Args:
            response: LLM 鍝嶅簲瀵硅薄
            question: 鍘熷闂 (鐢ㄤ簬鍥為€€)

        Returns:
            鏌ヨ瀛楃涓插垪琛? 瑙ｆ瀽澶辫触鍒欒繑鍥?[question]
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
        鍥為€€绛栫暐: 杩斿洖鍘熷闂浣滀负鏌ヨ / Fallback: use original question.

        褰?LLM 瑙ｆ瀽澶辫触鏃? 鑷冲皯杩斿洖鍘熷鏌ヨ,
        淇濊瘉绯荤粺鍦ㄥ紓甯告儏鍐典笅浠嶈兘宸ヤ綔銆?
        Args:
            question: 鍘熷鐢ㄦ埛闂

        Returns:
            鍖呭惈鍘熷闂鐨勫崟鍏冪礌鍒楄〃
        """
        return [question]
