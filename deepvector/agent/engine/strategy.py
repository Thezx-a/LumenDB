"""
妫€绱㈢瓥鐣ュ畾涔?/ Retrieval Strategy Definitions.

瀹氫箟 AgenticDB 鏀寔鐨勫洓绉嶆悳绱㈢瓥鐣ュ拰璁″垝鏁版嵁缁撴瀯銆?
绛栫暐瀵规瘮 / Strategy Comparison:
  Strategy      | 浣跨敤鍦烘櫙               | 杞暟 | 鏌ヨ鏁?  DIRECT        | 绠€鍗曚簨瀹炴€ч棶棰?        | 1    | 1
  FILTERED      | 甯︽潯浠剁害鏉熺殑鏌ヨ       | 1    | 1 + filter
  MULTI_QUERY   | 澶氳搴﹀鏉傞棶棰?        | 1-2  | 2-5
  HIERARCHICAL  | 寮€鏀炬€ф帰绱㈤棶棰?        | 2-5  | 姣忚疆1-2

璁捐鍘熷垯 / Design Principles:
  - dataclass 纭繚绫诲瀷瀹夊叏鍜屼笉鍙彉鎬?/ Type-safe immutable data structures
  - strategy 鏋氫妇淇濊瘉绛栫暐鍚嶇О涓€鑷存€?/ Enum for strategy names consistency
  - RetrievalPlan 鍙互琚簭鍒楀寲涓?JSON (鐢ㄤ簬鏃ュ織/鐩戞帶)
"""

import json
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Dict, List, Optional


class SearchStrategy(Enum):
    """
    妫€绱㈢瓥鐣ユ灇涓?/ Supported retrieval strategies.

    姣忕绛栫暐瀵瑰簲涓嶅悓鐨勬煡璇㈣涓?
    - DIRECT: 鍗曡疆璇箟鎼滅储, 鏈€蹇絾鏈€绠€鍗?    - FILTERED: 鍩轰簬鍏冩暟鎹殑杩囨护鎼滅储
    - MULTI_QUERY: 澶氭煡璇㈠苟琛?+ 缁撴灉铻嶅悎
    - HIERARCHICAL: 娓愯繘寮忔悳绱? 閫愭绮惧寲
    """
    DIRECT = "direct"           # 鐩存帴鎼滅储 / One-shot semantic search
    FILTERED = "filtered"       # 甯﹁繃婊?/ Search with metadata constraints
    MULTI_QUERY = "multi_query" # 澶氭煡璇?/ Multiple sub-queries
    HIERARCHICAL = "hierarchical"  # 灞傛鍖?/ Progressive refinement


@dataclass
class SearchStep:
    """
    鍗曟鎼滅储鎿嶄綔 / A single search operation.

    Attributes:
        query: 鎼滅储鏌ヨ鏂囨湰
        filter: 鍙€夊厓鏁版嵁杩囨护鏉′欢 (FilterNode JSON 鏍煎紡)
        k: 杩斿洖缁撴灉鏁伴噺
        collection: 闆嗗悎鍚嶇О
        round_num: 鎵€灞炶疆娆?(鐢卞紩鎿庤嚜鍔ㄨ缃?
    """
    query: str
    filter: Optional[Dict[str, Any]] = None
    k: int = 10
    collection: str = "default"
    round_num: int = 0


@dataclass
class RetrievalPlan:
    """
    妫€绱㈣鍒?/ A complete retrieval plan.

    鐢?QueryPlanner 鐢熸垚, 鍖呭惈绛栫暐閫夋嫨銆佹楠ゅ垪琛ㄥ拰棰勬湡杞暟銆?    鍙互琚紩鎿庢墽琛? 涔熷彲浠ュ簭鍒楀寲涓?JSON 鐢ㄤ簬鏃ュ織銆?
    Attributes:
        strategy: 閫変腑鐨勬悳绱㈢瓥鐣?        reasoning: LLM 閫夋嫨璇ョ瓥鐣ョ殑鍘熷洜 (鐢ㄤ簬璋冭瘯鍜岄潰璇曞睍绀?
        steps: 瑕佹墽琛岀殑鍏蜂綋鎼滅储姝ラ鍒楄〃
        expected_rounds: 棰勬湡鎵ц杞暟
        original_question: 鍘熷鐢ㄦ埛闂 (鐢ㄤ簬鍥炴函)
    """
    strategy: SearchStrategy
    reasoning: str
    steps: List[SearchStep] = field(default_factory=list)
    expected_rounds: int = 1
    original_question: str = ""

    def summary(self) -> str:
        """
        鐢熸垚浜虹被鍙鐨勮鍒掓憳瑕?/ Generate human-readable plan summary.

        鐢ㄤ簬鏃ュ織璁板綍鍜岃皟璇曡緭鍑恒€?
        Returns:
            澶氳瀛楃涓? 鍖呭惈绛栫暐銆佸師鍥犲拰姝ラ璇︽儏
        """
        parts = [f"Strategy: {self.strategy.value}"]
        parts.append(f"Reasoning: {self.reasoning}")
        parts.append(f"Steps: {len(self.steps)}, Expected rounds: {self.expected_rounds}")
        for i, step in enumerate(self.steps):
            f_str = json.dumps(step.filter) if step.filter else "none"
            parts.append(f"  Step {i+1}: query='{step.query}' filter={f_str} k={step.k}")
        return "\n".join(parts)
