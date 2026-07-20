"""
Function Calling Schemas — define tools available to the LLM.

These schemas follow the OpenAI Function Calling format and are compatible with Ollama.
The LLM decides which tool to call and with what parameters.

Available tools:
  - vector_search: basic semantic search
  - filtered_search: search with metadata filters
  - reformulate_queries: query reformulation for multi-round retrieval
"""

SEARCH_TOOL = {
    "type": "function",
    "function": {
        "name": "vector_search",
        "description": (
            "Search the vector database for documents semantically similar to a query. "
            "Uses semantic matching to find the most relevant content."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": (
                        "Search query text. Should contain key concepts and terms."
                    ),
                },
                "k": {
                    "type": "integer",
                    "description": (
                        "Number of results to return (default: 10, max: 100)."
                    ),
                    "default": 10,
                },
                "collection": {
                    "type": "string",
                    "description": (
                        "Collection name. Different collections store different data types."
                    ),
                    "default": "default",
                },
            },
            "required": ["query"],
        },
    },
}

FILTERED_SEARCH_TOOL = {
    "type": "function",
    "function": {
        "name": "filtered_search",
        "description": (
            "Search with metadata filters. Use when you need to narrow results "
            "by tags, categories, dates, or other metadata fields."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "The search query text",
                },
                "k": {
                    "type": "integer",
                    "description": "Number of results to return",
                    "default": 10,
                },
                "filter": {
                    "type": "object",
                    "description": (
                        "Filter conditions as a tree. "
                        "Example: {'op': 'and', 'children': ["
                        "{'op': 'eq', 'field': 'tags', 'value': 'topic:RAG'}, "
                        "{'op': 'gt', 'field': 'timestamp', 'value': '1700000000'}"
                        "]}"
                    ),
                },
                "collection": {
                    "type": "string",
                    "description": "Collection name",
                    "default": "default",
                },
            },
            "required": ["query", "filter"],
        },
    },
}

REFORMULATE_TOOL = {
    "type": "function",
    "function": {
        "name": "reformulate_queries",
        "description": (
            "Generate improved search queries based on what you've already found. "
            "Use when initial search results are insufficient."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "new_queries": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "List of improved search queries to try next.",
                },
                "reasoning": {
                    "type": "string",
                    "description": (
                        "Why these new queries should help find better results"
                    ),
                },
            },
            "required": ["new_queries", "reasoning"],
        },
    },
}

ALL_TOOLS = [SEARCH_TOOL, FILTERED_SEARCH_TOOL, REFORMULATE_TOOL]
