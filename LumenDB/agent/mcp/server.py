"""
MCP Server — 将 LumenDB 作为 MCP 工具暴露 / Expose LumenDB as MCP tools.

MCP (Model Context Protocol) 是 AI 代理框架与外部工具之间的标准协议。
通过 MCP Server, 任何支持 MCP 的 Agent 框架 (LangChain, AutoGen, etc.)
都可以即插即用地将 LumenDB 作为向量搜索工具调用。

协议实现 / Protocol Implementation:
  基于 JSON-RPC 2.0, 支持 stdio 和 SSE 两种传输模式。
  详见: https://modelcontextprotocol.io

工具清单 / Available Tools:
  1. vector_search       — 语义向量搜索
  2. filtered_search     — 带元数据过滤的搜索
  3. add_documents       — 批量添加文档 (自动嵌入索引)
  4. get_collection_info — 集合信息统计
  5. list_collections    — 列出所有集合
  6. delete_document     — 删除文档
"""

import json
import logging
from typing import Any, Dict, List

import httpx

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# MCP 工具定义 / MCP Tool Definitions
#   每个工具包含 name, description, inputSchema (JSON Schema 格式)
# ---------------------------------------------------------------------------

MCP_TOOLS = [
    {
        "name": "vector_search",
        "description": (
            "在 LumenDB 中搜索与查询语义相似的文档。返回最相关的结果。"
            "Search LumenDB for documents semantically similar to a query."
            " Returns the top-k most relevant documents."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "搜索查询文本 / The search query text",
                },
                "k": {
                    "type": "integer",
                    "description": "返回结果数量 (默认 10) / Number of results",
                    "default": 10,
                },
                "collection": {
                    "type": "string",
                    "description": "集合名称 (默认 'default') / Collection name",
                    "default": "default",
                },
            },
            "required": ["query"],
        },
    },
    {
        "name": "filtered_search",
        "description": (
            "带元数据过滤的搜索。当需要按标签、类别或字段筛选结果时使用。"
            "Search with metadata filters. Use to narrow results by fields."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "搜索查询文本"},
                "k": {"type": "integer", "default": 10},
                "filter": {
                    "type": "object",
                    "description": (
                        "过滤树。示例: {'op': 'eq', 'field': 'tags', 'value': 'RAG'}"
                    ),
                },
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["query", "filter"],
        },
    },
    {
        "name": "add_documents",
        "description": (
            "向数据库添加文档。文本将被自动嵌入和索引。"
            "Add documents to the database. Texts will be automatically embedded."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "texts": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "要添加的文本列表 / List of texts to add",
                },
                "metadatas": {
                    "type": "array",
                    "items": {"type": "object"},
                    "description": "可选的元数据, 与 texts 一一对应 / Optional metadata",
                },
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["texts"],
        },
    },
    {
        "name": "get_collection_info",
        "description": "获取集合信息 (大小、维度等) / Get collection information.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection": {"type": "string", "default": "default"},
            },
        },
    },
    {
        "name": "list_collections",
        "description": "列出所有可用集合 / List all available collections.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "delete_document",
        "description": "按 ID 删除文档 / Delete a document by ID.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "integer", "description": "文档 ID / Document ID"},
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["id"],
        },
    },
]


class LumenDBMCPTools:
    """
    LumenDB MCP 工具实现 / LumenDB MCP Tool Implementations.

    每个方法对应一个 MCP 工具, 通过 HTTP 调用 LumenDB C++ Server。

    注意: 当前 LumenDB 部分 API 尚不支持 (如 /embed, /collections),
    这些工具将随 LumenDB Server 增强而逐步完善。
    """

    def __init__(self, lumendb_url: str = "http://localhost:8080"):
        """
        初始化 MCP 工具 / Initialize MCP tools.

        Args:
            lumendb_url: LumenDB C++ Server 的 HTTP 地址
        """
        self.lumendb_url = lumendb_url
        self._client: httpx.AsyncClient | None = None

    async def _ensure_client(self):
        """确保 HTTP 客户端已初始化 / Ensure HTTP client is initialized."""
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    async def vector_search(
        self, query: str, k: int = 10, collection: str = "default"
    ) -> str:
        """
        语义向量搜索 / Semantic vector search.

        流程: 嵌入查询 → POST /search → 返回 JSON 结果

        Args:
            query: 搜索查询文本
            k: 返回结果数量
            collection: 集合名称

        Returns:
            JSON 格式的结果字符串
        """
        await self._ensure_client()
        embed_resp = await self._client.post(
            f"{self.lumendb_url}/embed",
            json={"text": query},
        )
        embed_resp.raise_for_status()
        vector = embed_resp.json()["vector"]

        resp = await self._client.post(
            f"{self.lumendb_url}/search",
            json={"vector": vector, "k": k},
        )
        resp.raise_for_status()
        results = resp.json()["results"]

        return json.dumps({"results": results, "count": len(results)}, indent=2)

    async def filtered_search(
        self,
        query: str,
        filter: Dict[str, Any],
        k: int = 10,
        collection: str = "default",
    ) -> str:
        """
        带过滤的搜索 / Search with metadata filters.

        Args:
            query: 搜索查询文本
            filter: 过滤条件树 / Filter condition tree
            k: 返回结果数量
            collection: 集合名称

        Returns:
            JSON 格式的结果字符串
        """
        await self._ensure_client()
        embed_resp = await self._client.post(
            f"{self.lumendb_url}/embed",
            json={"text": query},
        )
        embed_resp.raise_for_status()
        vector = embed_resp.json()["vector"]

        resp = await self._client.post(
            f"{self.lumendb_url}/search",
            json={"vector": vector, "k": k, "filter": filter},
        )
        resp.raise_for_status()
        results = resp.json()["results"]

        return json.dumps({"results": results, "count": len(results)}, indent=2)

    async def add_documents(
        self,
        texts: List[str],
        metadatas: List[Dict[str, Any]] | None = None,
        collection: str = "default",
    ) -> str:
        """
        批量添加文档 / Add documents to the database.

        Args:
            texts: 文本列表
            metadatas: 元数据列表 (可选)
            collection: 集合名称

        Returns:
            JSON 格式的结果字符串
        """
        await self._ensure_client()
        resp = await self._client.post(
            f"{self.lumendb_url}/embed_and_insert",
            json={"texts": texts, "metadatas": metadatas},
        )
        resp.raise_for_status()
        return json.dumps(resp.json())

    async def get_collection_info(self, collection: str = "default") -> str:
        """
        获取集合信息 / Get collection information.

        Args:
            collection: 集合名称

        Returns:
            JSON 格式的集合信息
        """
        await self._ensure_client()
        resp = await self._client.get(f"{self.lumendb_url}/health")
        resp.raise_for_status()
        return json.dumps(resp.json())

    async def list_collections(self) -> str:
        """
        列出所有集合 / List all collections.

        Returns:
            JSON 格式的集合列表
        """
        await self._ensure_client()
        resp = await self._client.get(f"{self.lumendb_url}/collections")
        resp.raise_for_status()
        return json.dumps(resp.json())

    async def delete_document(self, id: int, collection: str = "default") -> str:
        """
        删除文档 / Delete a document by ID.

        Args:
            id: 文档 ID
            collection: 集合名称

        Returns:
            JSON 格式的删除结果
        """
        await self._ensure_client()
        resp = await self._client.delete(f"{self.lumendb_url}/vectors/{id}")
        resp.raise_for_status()
        return json.dumps({"status": "deleted", "id": id})

    async def close(self):
        """关闭 HTTP 客户端 / Close HTTP client."""
        if self._client and not self._client.is_closed:
            await self._client.aclose()


async def create_mcp_server(lumendb_url: str = "http://localhost:8080"):
    """
    创建 MCP 服务器实例 / Create an MCP server instance with LumenDB tools.

    使用官方 mcp Python SDK 创建标准 MCP Server。
    提供工具列表和工具调用处理。

    Args:
        lumendb_url: LumenDB HTTP API 地址

    Returns:
        (MCP Server 实例, MCP 工具实例) 元组

    Raises:
        ImportError: 如果 mcp 包未安装
    """
    try:
        from mcp.server import Server
        from mcp.types import Tool, TextContent

        server = Server("lumendb")
        tools = LumenDBMCPTools(lumendb_url)

        @server.list_tools()
        async def list_tools():
            return [Tool(**t) for t in MCP_TOOLS]

        @server.call_tool()
        async def call_tool(name: str, arguments: Dict[str, Any]):
            handlers = {
                "vector_search": lambda args: tools.vector_search(
                    args["query"],
                    args.get("k", 10),
                    args.get("collection", "default"),
                ),
                "filtered_search": lambda args: tools.filtered_search(
                    args["query"],
                    args["filter"],
                    args.get("k", 10),
                    args.get("collection", "default"),
                ),
                "add_documents": lambda args: tools.add_documents(
                    args["texts"],
                    args.get("metadatas"),
                    args.get("collection", "default"),
                ),
                "get_collection_info": lambda args: tools.get_collection_info(
                    args.get("collection", "default")
                ),
                "list_collections": lambda args: tools.list_collections(),
                "delete_document": lambda args: tools.delete_document(
                    args["id"],
                    args.get("collection", "default"),
                ),
            }

            if name not in handlers:
                return [TextContent(type="text", text=f"Unknown tool: {name}")]

            result = await handlers[name](arguments)
            return [TextContent(type="text", text=result)]

        return server, tools

    except ImportError:
        logger.error(
            "mcp package not installed. Install with: pip install mcp"
        )
        raise
