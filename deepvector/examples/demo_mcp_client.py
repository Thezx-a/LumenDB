"""
MCP Client Demo 鈥?shows how to call DeepVector via the MCP protocol.

This demonstrates the MCP server capabilities for agent frameworks.
"""

import asyncio
import json
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


async def demo_mcp_client():
    """Demo MCP client that calls DeepVector tools."""
    print("=" * 60)
    print("MCP-DeepVector Demo")
    print("=" * 60)

    try:
        from mcp.client import Client
        print("\n[OK] MCP SDK available")

        # In a real scenario, you'd connect to the MCP server:
        # client = Client("http://localhost:8090/mcp")
        # tools = await client.list_tools()

        print("\nAvailable MCP Tools:")
        print("  vector_search    - Semantic vector search")
        print("  filtered_search  - Search with metadata filters")
        print("  add_documents    - Add documents with text")
        print("  get_collection_info - Collection statistics")
        print("  list_collections - List all collections")
        print("  delete_document  - Delete a document")

        print("\nExample usage:")
        print("""
  # Via mcp CLI:
  mcp call lumendb vector_search '{"query": "What is RAG?", "k": 5}'

  # Via Python SDK:
  result = await client.call_tool("vector_search", {
      "query": "HNSW indexing algorithm",
      "k": 10
  })
  print(result.content[0].text)
        """)

    except ImportError:
        print("\n[!] MCP SDK not installed. Install with: pip install mcp")
        print("\nThe server is ready but cannot be demonstrated without the SDK.")


async def main():
    await demo_mcp_client()


if __name__ == "__main__":
    asyncio.run(main())
