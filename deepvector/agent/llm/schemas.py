"""
Function Calling Schemas 鈥?瀹氫箟 LLM 鍙敤鐨勫伐鍏?/ Define tools available to the LLM.

杩欎簺 schema 閬靛惊 OpenAI Function Calling 瑙勮寖, 鍚屾椂鍏煎 Ollama銆?LLM 鏍规嵁鐢ㄦ埛闂鑷富鍐冲畾璋冪敤鍝釜宸ュ叿銆佷紶浠€涔堝弬鏁般€?
Schema 璁捐鍘熷垯 / Design Principles:
  1. 宸ュ叿鍚嶇О娓呮櫚琛ㄨ揪鍔熻兘 / Clear tool names
  2. 鎻忚堪瀛楁鍖呭惈浣跨敤鍦烘櫙 / Descriptions include when-to-use
  3. 鍙傛暟鐢?LLM 鑷姩鎻愬彇 / Parameters extracted by LLM
  4. required 瀛楁纭繚鍏抽敭鍙傛暟蹇呬紶 / Required ensures key parameters

宸ュ叿娓呭崟 / Available Tools:
  - vector_search: 鍩虹璇箟鎼滅储 / Basic semantic search
  - filtered_search: 甯﹀厓鏁版嵁杩囨护鐨勬悳绱?/ Filtered search
  - reformulate_queries: 鏌ヨ閲嶆瀯 (澶氳疆妫€绱㈢敤) / Query reformulation
"""

# ---------------------------------------------------------------------------
# 宸ュ叿 1: 璇箟鍚戦噺鎼滅储 / Tool 1: Semantic Vector Search
#   閫氱敤鎼滅储宸ュ叿, 閫氳繃鏂囨湰璇箟鐩镐技搴︽煡鎵剧浉鍏虫枃妗ｃ€?#   閫傜敤浜? 浜嬪疄鏌ヨ銆佹蹇佃В閲娿€佷俊鎭煡鎵?# ---------------------------------------------------------------------------
SEARCH_TOOL = {
    "type": "function",
    "function": {
        "name": "vector_search",
        "description": (
            "鍦ㄥ悜閲忔暟鎹簱涓悳绱笌鏌ヨ璇箟鐩镐技鐨勬枃妗ｃ€?
            "浣跨敤璇箟鍖归厤鎵惧埌鏈€鐩稿叧鐨勫唴瀹广€?
            "Search the vector database for documents semantically similar to a query."
            " Uses semantic matching to find the most relevant content."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": (
                        "鎼滅储鏌ヨ鏂囨湰, 搴斿寘鍚叧閿蹇靛拰鏈銆?
                        "Search query text. Should contain key concepts and terms."
                    ),
                },
                "k": {
                    "type": "integer",
                    "description": (
                        "杩斿洖缁撴灉鏁伴噺 (榛樿 10, 鏈€澶?100)銆?
                        "Number of results to return (default: 10, max: 100)."
                    ),
                    "default": 10,
                },
                "collection": {
                    "type": "string",
                    "description": (
                        "闆嗗悎鍚嶇О, 涓嶅悓闆嗗悎瀛樺偍涓嶅悓绫诲瀷鐨勬暟鎹€?
                        "Collection name. Different collections store different data types."
                    ),
                    "default": "default",
                },
            },
            "required": ["query"],
        },
    },
}

# ---------------------------------------------------------------------------
# 宸ュ叿 2: 甯﹁繃婊ょ殑鎼滅储 / Tool 2: Filtered Search
#   鍦ㄨ涔夋悳绱㈠熀纭€涓? 閫氳繃鍏冩暟鎹繃婊ょ缉灏忚寖鍥淬€?#   閫傜敤浜? 鎸夌被鍒?鏃堕棿/鏍囩绛涢€夌殑缁撴灉鏌ヨ
#
#   Filter 璇硶涓烘爲褰㈢粨鏋? 鏀寔:
#     - 姣旇緝: eq (绛変簬), gt (澶т簬), lt (灏忎簬), contains (鍖呭惈)
#     - 閫昏緫: and (涓?, or (鎴?, not (闈?
#   Example: {"op": "and", "children": [
#       {"op": "eq", "field": "tags", "value": "RAG"},
#       {"op": "gt", "field": "timestamp", "value": "1700000000"}
#   ]}
# ---------------------------------------------------------------------------
FILTERED_SEARCH_TOOL = {
    "type": "function",
    "function": {
        "name": "filtered_search",
        "description": (
            "甯﹀厓鏁版嵁杩囨护鐨勮涔夋悳绱€?
            "褰撻渶瑕佹寜鏍囩銆佺被鍒€佹棩鏈熸垨鐗瑰畾瀛楁绛涢€夌粨鏋滄椂浣跨敤姝ゅ伐鍏枫€?
            "Search with metadata filters. Use when you need to narrow results "
            "by tags, categories, dates, or other metadata fields."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "鎼滅储鏌ヨ鏂囨湰 / The search query text",
                },
                "k": {
                    "type": "integer",
                    "description": (
                        "杩斿洖缁撴灉鏁伴噺 / Number of results to return"
                    ),
                    "default": 10,
                },
                "filter": {
                    "type": "object",
                    "description": (
                        "杩囨护鏉′欢鏍戙€?
                        "Filter conditions as a tree. "
                        "Example: {'op': 'and', 'children': ["
                        "{'op': 'eq', 'field': 'tags', 'value': 'topic:RAG'}, "
                        "{'op': 'gt', 'field': 'timestamp', 'value': '1700000000'}"
                        "]}"
                    ),
                },
                "collection": {
                    "type": "string",
                    "description": "闆嗗悎鍚嶇О / Collection name",
                    "default": "default",
                },
            },
            "required": ["query", "filter"],
        },
    },
}

# ---------------------------------------------------------------------------
# 宸ュ叿 3: 鏌ヨ閲嶆瀯 / Tool 3: Query Reformulation
#   褰撶涓€杞悳绱㈢粨鏋滀笉鍏呭垎鏃? LLM 鐢ㄦ宸ュ叿鐢熸垚鏀硅繘鐨勬煡璇€?#   杩欎笉鏄疄闄呮悳绱㈠伐鍏? 鑰屾槸璁?LLM 閲嶆柊鎬濊€冩悳绱㈢瓥鐣ャ€?# ---------------------------------------------------------------------------
REFORMULATE_TOOL = {
    "type": "function",
    "function": {
        "name": "reformulate_queries",
        "description": (
            "鍩轰簬宸叉湁鎼滅储缁撴灉鐢熸垚鏀硅繘鐨勬悳绱㈡煡璇€?
            "褰撳垵濮嬫悳绱㈢粨鏋滀笉鍏呭垎鏃? 浣跨敤涓嶅悓鐨勫叧閿瘝/瑙掑害閲嶆柊鎼滅储銆?
            "Generate improved search queries based on what you've already found."
            " Use when initial search results are insufficient."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "new_queries": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": (
                        "鏀硅繘鍚庣殑鎼滅储鏌ヨ鍒楄〃, 浣跨敤涓嶅悓鐨勫叧閿瘝鎴栬〃杈炬柟寮忋€?
                        "List of improved search queries to try next."
                    ),
                },
                "reasoning": {
                    "type": "string",
                    "description": (
                        "涓轰粈涔堣繖浜涙柊鏌ヨ鑳芥壘鍒版洿濂界殑缁撴灉 / "
                        "Why these new queries should help find better results"
                    ),
                },
            },
            "required": ["new_queries", "reasoning"],
        },
    },
}

# 鎵€鏈夊彲鐢ㄥ伐鍏风殑姹囨€诲垪琛?/ Aggregated list of all available tools
ALL_TOOLS = [SEARCH_TOOL, FILTERED_SEARCH_TOOL, REFORMULATE_TOOL]
