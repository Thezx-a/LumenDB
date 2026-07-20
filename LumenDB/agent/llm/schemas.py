"""
Function Calling Schemas — 定义 LLM 可用的工具 / Define tools available to the LLM.

这些 schema 遵循 OpenAI Function Calling 规范, 同时兼容 Ollama。
LLM 根据用户问题自主决定调用哪个工具、传什么参数。

Schema 设计原则 / Design Principles:
  1. 工具名称清晰表达功能 / Clear tool names
  2. 描述字段包含使用场景 / Descriptions include when-to-use
  3. 参数由 LLM 自动提取 / Parameters extracted by LLM
  4. required 字段确保关键参数必传 / Required ensures key parameters

工具清单 / Available Tools:
  - vector_search: 基础语义搜索 / Basic semantic search
  - filtered_search: 带元数据过滤的搜索 / Filtered search
  - reformulate_queries: 查询重构 (多轮检索用) / Query reformulation
"""

# ---------------------------------------------------------------------------
# 工具 1: 语义向量搜索 / Tool 1: Semantic Vector Search
#   通用搜索工具, 通过文本语义相似度查找相关文档。
#   适用于: 事实查询、概念解释、信息查找
# ---------------------------------------------------------------------------
SEARCH_TOOL = {
    "type": "function",
    "function": {
        "name": "vector_search",
        "description": (
            "在向量数据库中搜索与查询语义相似的文档。"
            "使用语义匹配找到最相关的内容。"
            "Search the vector database for documents semantically similar to a query."
            " Uses semantic matching to find the most relevant content."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": (
                        "搜索查询文本, 应包含关键概念和术语。"
                        "Search query text. Should contain key concepts and terms."
                    ),
                },
                "k": {
                    "type": "integer",
                    "description": (
                        "返回结果数量 (默认 10, 最大 100)。"
                        "Number of results to return (default: 10, max: 100)."
                    ),
                    "default": 10,
                },
                "collection": {
                    "type": "string",
                    "description": (
                        "集合名称, 不同集合存储不同类型的数据。"
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
# 工具 2: 带过滤的搜索 / Tool 2: Filtered Search
#   在语义搜索基础上, 通过元数据过滤缩小范围。
#   适用于: 按类别/时间/标签筛选的结果查询
#
#   Filter 语法为树形结构, 支持:
#     - 比较: eq (等于), gt (大于), lt (小于), contains (包含)
#     - 逻辑: and (与), or (或), not (非)
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
            "带元数据过滤的语义搜索。"
            "当需要按标签、类别、日期或特定字段筛选结果时使用此工具。"
            "Search with metadata filters. Use when you need to narrow results "
            "by tags, categories, dates, or other metadata fields."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "搜索查询文本 / The search query text",
                },
                "k": {
                    "type": "integer",
                    "description": (
                        "返回结果数量 / Number of results to return"
                    ),
                    "default": 10,
                },
                "filter": {
                    "type": "object",
                    "description": (
                        "过滤条件树。"
                        "Filter conditions as a tree. "
                        "Example: {'op': 'and', 'children': ["
                        "{'op': 'eq', 'field': 'tags', 'value': 'topic:RAG'}, "
                        "{'op': 'gt', 'field': 'timestamp', 'value': '1700000000'}"
                        "]}"
                    ),
                },
                "collection": {
                    "type": "string",
                    "description": "集合名称 / Collection name",
                    "default": "default",
                },
            },
            "required": ["query", "filter"],
        },
    },
}

# ---------------------------------------------------------------------------
# 工具 3: 查询重构 / Tool 3: Query Reformulation
#   当第一轮搜索结果不充分时, LLM 用此工具生成改进的查询。
#   这不是实际搜索工具, 而是让 LLM 重新思考搜索策略。
# ---------------------------------------------------------------------------
REFORMULATE_TOOL = {
    "type": "function",
    "function": {
        "name": "reformulate_queries",
        "description": (
            "基于已有搜索结果生成改进的搜索查询。"
            "当初始搜索结果不充分时, 使用不同的关键词/角度重新搜索。"
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
                        "改进后的搜索查询列表, 使用不同的关键词或表达方式。"
                        "List of improved search queries to try next."
                    ),
                },
                "reasoning": {
                    "type": "string",
                    "description": (
                        "为什么这些新查询能找到更好的结果 / "
                        "Why these new queries should help find better results"
                    ),
                },
            },
            "required": ["new_queries", "reasoning"],
        },
    },
}

# 所有可用工具的汇总列表 / Aggregated list of all available tools
ALL_TOOLS = [SEARCH_TOOL, FILTERED_SEARCH_TOOL, REFORMULATE_TOOL]
