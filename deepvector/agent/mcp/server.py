"""
MCP Server 鈥?灏?DeepVector 浣滀负 MCP 宸ュ叿鏆撮湶 / Expose DeepVector as MCP tools.

MCP (Model Context Protocol) 鏄?AI 浠ｇ悊妗嗘灦涓庡閮ㄥ伐鍏蜂箣闂寸殑鏍囧噯鍗忚銆?閫氳繃 MCP Server, 浠讳綍鏀寔 MCP 鐨?Agent 妗嗘灦 (LangChain, AutoGen, etc.)
閮藉彲浠ュ嵆鎻掑嵆鐢ㄥ湴灏?DeepVector 浣滀负鍚戦噺鎼滅储宸ュ叿璋冪敤銆?
鍗忚瀹炵幇 / Protocol Implementation:
  鍩轰簬 JSON-RPC 2.0, 鏀寔 stdio 鍜?SSE 涓ょ浼犺緭妯″紡銆?  璇﹁: https://modelcontextprotocol.io

宸ュ叿娓呭崟 / Available Tools:
  1. vector_search       鈥?璇箟鍚戦噺鎼滅储
  2. filtered_search     鈥?甯﹀厓鏁版嵁杩囨护鐨勬悳绱?  3. add_documents       鈥?鎵归噺娣诲姞鏂囨。 (鑷姩宓屽叆绱㈠紩)
  4. get_collection_info 鈥?闆嗗悎淇℃伅缁熻
  5. list_collections    鈥?鍒楀嚭鎵€鏈夐泦鍚?  6. delete_document     鈥?鍒犻櫎鏂囨。
"""

import json
import logging
from typing import Any, Dict, List

import httpx

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# MCP 宸ュ叿瀹氫箟 / MCP Tool Definitions
#   姣忎釜宸ュ叿鍖呭惈 name, description, inputSchema (JSON Schema 鏍煎紡)
# ---------------------------------------------------------------------------

MCP_TOOLS = [
    {
        "name": "vector_search",
        "description": (
            "鍦?DeepVector 涓悳绱笌鏌ヨ璇箟鐩镐技鐨勬枃妗ｃ€傝繑鍥炴渶鐩稿叧鐨勭粨鏋溿€?
            "Search DeepVector for documents semantically similar to a query."
            " Returns the top-k most relevant documents."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "鎼滅储鏌ヨ鏂囨湰 / The search query text",
                },
                "k": {
                    "type": "integer",
                    "description": "杩斿洖缁撴灉鏁伴噺 (榛樿 10) / Number of results",
                    "default": 10,
                },
                "collection": {
                    "type": "string",
                    "description": "闆嗗悎鍚嶇О (榛樿 'default') / Collection name",
                    "default": "default",
                },
            },
            "required": ["query"],
        },
    },
    {
        "name": "filtered_search",
        "description": (
            "甯﹀厓鏁版嵁杩囨护鐨勬悳绱€傚綋闇€瑕佹寜鏍囩銆佺被鍒垨瀛楁绛涢€夌粨鏋滄椂浣跨敤銆?
            "Search with metadata filters. Use to narrow results by fields."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "鎼滅储鏌ヨ鏂囨湰"},
                "k": {"type": "integer", "default": 10},
                "filter": {
                    "type": "object",
                    "description": (
                        "杩囨护鏍戙€傜ず渚? {'op': 'eq', 'field': 'tags', 'value': 'RAG'}"
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
            "鍚戞暟鎹簱娣诲姞鏂囨。銆傛枃鏈皢琚嚜鍔ㄥ祵鍏ュ拰绱㈠紩銆?
            "Add documents to the database. Texts will be automatically embedded."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "texts": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "瑕佹坊鍔犵殑鏂囨湰鍒楄〃 / List of texts to add",
                },
                "metadatas": {
                    "type": "array",
                    "items": {"type": "object"},
                    "description": "鍙€夌殑鍏冩暟鎹? 涓?texts 涓€涓€瀵瑰簲 / Optional metadata",
                },
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["texts"],
        },
    },
    {
        "name": "get_collection_info",
        "description": "鑾峰彇闆嗗悎淇℃伅 (澶у皬銆佺淮搴︾瓑) / Get collection information.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection": {"type": "string", "default": "default"},
            },
        },
    },
    {
        "name": "list_collections",
        "description": "鍒楀嚭鎵€鏈夊彲鐢ㄩ泦鍚?/ List all available collections.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "delete_document",
        "description": "鎸?ID 鍒犻櫎鏂囨。 / Delete a document by ID.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "integer", "description": "鏂囨。 ID / Document ID"},
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["id"],
        },
    },
]


class DeepVectorMCPTools:
    """
    DeepVector MCP 宸ュ叿瀹炵幇 / DeepVector MCP Tool Implementations.

    姣忎釜鏂规硶瀵瑰簲涓€涓?MCP 宸ュ叿, 閫氳繃 HTTP 璋冪敤 DeepVector C++ Server銆?
    娉ㄦ剰: 褰撳墠 DeepVector 閮ㄥ垎 API 灏氫笉鏀寔 (濡?/embed, /collections),
    杩欎簺宸ュ叿灏嗛殢 DeepVector Server 澧炲己鑰岄€愭瀹屽杽銆?    """

    def __init__(self, lumendb_url: str = "http://localhost:8080"):
        """
        鍒濆鍖?MCP 宸ュ叿 / Initialize MCP tools.

        Args:
            lumendb_url: DeepVector C++ Server 鐨?HTTP 鍦板潃
        """
        self.lumendb_url = lumendb_url
        self._client: httpx.AsyncClient | None = None

    async def _ensure_client(self):
        """纭繚 HTTP 瀹㈡埛绔凡鍒濆鍖?/ Ensure HTTP client is initialized."""
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    async def vector_search(
        self, query: str, k: int = 10, collection: str = "default"
    ) -> str:
        """
        璇箟鍚戦噺鎼滅储 / Semantic vector search.

        娴佺▼: 宓屽叆鏌ヨ 鈫?POST /search 鈫?杩斿洖 JSON 缁撴灉

        Args:
            query: 鎼滅储鏌ヨ鏂囨湰
            k: 杩斿洖缁撴灉鏁伴噺
            collection: 闆嗗悎鍚嶇О

        Returns:
            JSON 鏍煎紡鐨勭粨鏋滃瓧绗︿覆
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
        甯﹁繃婊ょ殑鎼滅储 / Search with metadata filters.

        Args:
            query: 鎼滅储鏌ヨ鏂囨湰
            filter: 杩囨护鏉′欢鏍?/ Filter condition tree
            k: 杩斿洖缁撴灉鏁伴噺
            collection: 闆嗗悎鍚嶇О

        Returns:
            JSON 鏍煎紡鐨勭粨鏋滃瓧绗︿覆
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
        鎵归噺娣诲姞鏂囨。 / Add documents to the database.

        Args:
            texts: 鏂囨湰鍒楄〃
            metadatas: 鍏冩暟鎹垪琛?(鍙€?
            collection: 闆嗗悎鍚嶇О

        Returns:
            JSON 鏍煎紡鐨勭粨鏋滃瓧绗︿覆
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
        鑾峰彇闆嗗悎淇℃伅 / Get collection information.

        Args:
            collection: 闆嗗悎鍚嶇О

        Returns:
            JSON 鏍煎紡鐨勯泦鍚堜俊鎭?        """
        await self._ensure_client()
        resp = await self._client.get(f"{self.lumendb_url}/health")
        resp.raise_for_status()
        return json.dumps(resp.json())

    async def list_collections(self) -> str:
        """
        鍒楀嚭鎵€鏈夐泦鍚?/ List all collections.

        Returns:
            JSON 鏍煎紡鐨勯泦鍚堝垪琛?        """
        await self._ensure_client()
        resp = await self._client.get(f"{self.lumendb_url}/collections")
        resp.raise_for_status()
        return json.dumps(resp.json())

    async def delete_document(self, id: int, collection: str = "default") -> str:
        """
        鍒犻櫎鏂囨。 / Delete a document by ID.

        Args:
            id: 鏂囨。 ID
            collection: 闆嗗悎鍚嶇О

        Returns:
            JSON 鏍煎紡鐨勫垹闄ょ粨鏋?        """
        await self._ensure_client()
        resp = await self._client.delete(f"{self.lumendb_url}/vectors/{id}")
        resp.raise_for_status()
        return json.dumps({"status": "deleted", "id": id})

    async def close(self):
        """鍏抽棴 HTTP 瀹㈡埛绔?/ Close HTTP client."""
        if self._client and not self._client.is_closed:
            await self._client.aclose()


async def create_mcp_server(lumendb_url: str = "http://localhost:8080"):
    """
    鍒涘缓 MCP 鏈嶅姟鍣ㄥ疄渚?/ Create an MCP server instance with DeepVector tools.

    浣跨敤瀹樻柟 mcp Python SDK 鍒涘缓鏍囧噯 MCP Server銆?    鎻愪緵宸ュ叿鍒楄〃鍜屽伐鍏疯皟鐢ㄥ鐞嗐€?
    Args:
        lumendb_url: DeepVector HTTP API 鍦板潃

    Returns:
        (MCP Server 瀹炰緥, MCP 宸ュ叿瀹炰緥) 鍏冪粍

    Raises:
        ImportError: 濡傛灉 mcp 鍖呮湭瀹夎
    """
    try:
        from mcp.server import Server
        from mcp.types import Tool, TextContent

        server = Server("lumendb")
        tools = DeepVectorMCPTools(lumendb_url)

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
