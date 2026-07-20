"""
鏌ヨ瑙勫垝鍣?/ Query Planner 鈥?鑷劧璇█闂 鈫?妫€绱㈢瓥鐣ャ€?
鏍稿績娴佺▼ / Core Flow:
  1. 鎺ユ敹鐢ㄦ埛鑷劧璇█闂
  2. 璋冪敤 LLM 鍒嗘瀽闂绫诲瀷
  3. LLM 閫夋嫨绛栫暐 (DIRECT/FILTERED/MULTI_QUERY/HIERARCHICAL)
  4. 鐢熸垚缁撴瀯鍖栫殑 RetrievalPlan

鏋舵瀯娉ㄦ剰 / Architecture Note:
  鏌ヨ瑙勫垝鏄?AgenticDB 鍖哄埆浜庢櫘閫氬悜閲忔暟鎹簱鐨勬牳蹇冪壒鎬с€?  浼犵粺鍚戦噺鏁版嵁搴撻渶瑕佸紑鍙戣€呮墜鍔ㄧ紪鐮佹悳绱㈤€昏緫,
  AgenticDB 璁?LLM 鑷姩瀹屾垚杩欎釜鍐崇瓥杩囩▼銆?"""

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
    鏌ヨ瑙勫垝鍣?/ Query Planner.

    鍒嗘瀽鐢ㄦ埛鐨勮嚜鐒惰瑷€闂, 鐢熸垚缁撴瀯鍖栫殑妫€绱㈣鍒掋€?    浣跨敤 LLM 鐨勮涔夌悊瑙ｈ兘鍔涘拰宸ュ叿璋冪敤鑳藉姏銆?
    鐢ㄦ硶 / Usage:
        planner = QueryPlanner(llm)
        plan = await planner.plan("What is RAG?")
        print(plan.strategy)  # SearchStrategy.DIRECT
    """

    def __init__(self, llm: LLMRouter, default_collection: str = "default"):
        """
        鍒濆鍖栨煡璇㈣鍒掑櫒 / Initialize query planner.

        Args:
            llm: LLM 璺敱鍣ㄥ疄渚?            default_collection: 榛樿闆嗗悎鍚嶇О
        """
        self.llm = llm
        self.default_collection = default_collection

    async def plan(
        self, question: str, collection: Optional[str] = None
    ) -> RetrievalPlan:
        """
        鍒嗘瀽闂骞剁敓鎴愭绱㈣鍒?/ Analyze question and generate retrieval plan.

        姝ラ:
          1. 灏嗛棶棰樺拰 system prompt 涓€璧峰彂閫佺粰 LLM
          2. 鎻愪緵宸ュ叿瀹氫箟璁?LLM 鍙互閫夋嫨璋冪敤
          3. 浼樺厛灏濊瘯浠?tool_call 鎻愬彇璁″垝 (鏇村彲闈?
          4. 鍥為€€鍒拌В鏋?JSON content

        Args:
            question: 鐢ㄦ埛鐨勮嚜鐒惰瑷€闂
            collection: 闆嗗悎鍚嶇О (鍙€?

        Returns:
            RetrievalPlan 鍖呭惈绛栫暐銆佹楠ゅ拰棰勬湡杞暟
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

        # 绛栫暐 1: 浼樺厛浠?tool_call 鎻愬彇 / Priority: extract from tool calls
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

        # 绛栫暐 2: 浠?JSON content 瑙ｆ瀽 / Fallback: parse JSON from content
        return self._plan_from_json(response.content, question, coll)

    def _plan_from_single_search(
        self, args: Dict[str, Any], question: str, collection: str
    ) -> RetrievalPlan:
        """
        浠?vector_search 宸ュ叿璋冪敤鐢熸垚璁″垝 / Extract plan from vector_search tool call.

        褰?LLM 鐩存帴閫夋嫨浜?vector_search 宸ュ叿鏃?
        鎴戜滑灏嗗叾鍙傛暟鏄犲皠涓?DIRECT 绛栫暐鐨?RetrievalPlan銆?
        Args:
            args: vector_search 鐨勫弬鏁?(query, k, collection)
            question: 鍘熷鐢ㄦ埛闂
            collection: 闆嗗悎鍚嶇О

        Returns:
            DIRECT 绛栫暐鐨勬绱㈣鍒?        """
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
        浠?filtered_search 宸ュ叿璋冪敤鐢熸垚璁″垝 / Extract plan from filtered_search call.

        Args:
            args: filtered_search 鐨勫弬鏁?(query, filter, k, collection)
            question: 鍘熷鐢ㄦ埛闂
            collection: 闆嗗悎鍚嶇О

        Returns:
            FILTERED 绛栫暐鐨勬绱㈣鍒?        """
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
        浠?JSON 鏂囨湰瑙ｆ瀽璁″垝 / Parse plan from JSON text.

        褰?LLM 娌℃湁閫夋嫨 tool_call 鏃? 鎴戜滑鏈熸湜瀹冭繑鍥炰簡 JSON 鏍煎紡鐨勮鍒掋€?        闇€瑕佸鐞?markdown 浠ｇ爜鍧楀寘瑁呭拰 JSON 瑙ｆ瀽銆?
        Args:
            content: LLM 杩斿洖鐨勬枃鏈?(搴斿寘鍚?JSON)
            question: 鍘熷鐢ㄦ埛闂
            collection: 闆嗗悎鍚嶇О

        Returns:
            瑙ｆ瀽鍚庣殑 RetrievalPlan, 瑙ｆ瀽澶辫触鍒欏洖閫€鍒?DIRECT
        """
        try:
            # 鍘婚櫎鍙兘瀛樺湪鐨?markdown 浠ｇ爜鍧楀洿鏍?/ Strip markdown fences
            text = content.strip()
            if "```" in text:
                text = text.split("```")[1]
                if text.startswith("json"):
                    text = text[4:]
                text = text.strip()

            plan_data = json.loads(text)
            strategy_str = plan_data.get("strategy", "DIRECT").upper()

            # 瀛楃涓插埌鏋氫妇鐨勬槧灏?/ String to enum mapping
            strategy_map = {
                "DIRECT": SearchStrategy.DIRECT,
                "FILTERED": SearchStrategy.FILTERED,
                "MULTI_QUERY": SearchStrategy.MULTI_QUERY,
                "HIERARCHICAL": SearchStrategy.HIERARCHICAL,
            }
            strategy = strategy_map.get(strategy_str, SearchStrategy.DIRECT)

            # 瑙ｆ瀽鎼滅储姝ラ / Parse search steps
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
        鍥為€€璁″垝 / Fallback plan.

        褰?LLM 杩斿洖鍐呭鏃犳硶瑙ｆ瀽鏃朵娇鐢ㄣ€?        淇濊瘉绯荤粺鍦ㄤ换浣曟儏鍐典笅閮借兘姝ｅ父宸ヤ綔銆?
        Args:
            question: 鐢ㄦ埛闂
            collection: 闆嗗悎鍚嶇О

        Returns:
            鏈€绠€鍗曠殑 DIRECT 绛栫暐璁″垝
        """
        return RetrievalPlan(
            strategy=SearchStrategy.DIRECT,
            reasoning="Fallback: LLM response parsing failed",
            steps=[SearchStep(query=question, k=10, collection=collection)],
            expected_rounds=1,
            original_question=question,
        )
