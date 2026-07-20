# 第八章：MCP 服务器

> MCP (Model Context Protocol) — 让 Agent 框架即插即用 LumenDB。

## 前置知识

> 📎 **参考**: [Python环境](../prerequisites/02_Python环境_zh.md)

---

## 学习目标

- 理解 MCP 协议的核心概念
- 掌握 MCP Server 的实现
- 学会通过 MCP 集成 Agent 框架

---

## 8.1 什么是 MCP？

MCP (Model Context Protocol) 是 AI Agent 领域的"USB 接口"标准协议。

```
传统方式:
  Agent Framework → 适配器 → LumenDB SDK → LumenDB
                      ↑
                  每个框架都要写

MCP 方式:
  Agent Framework → MCP Client → MCP Server → LumenDB
                                    ↑
                              一次实现，到处使用
```

---

## 8.2 MCP 工具定义

AgenticDB 通过 MCP 暴露 6 个工具：

| 工具名 | 功能 | 参数 |
|--------|------|------|
| vector_search | 语义搜索 | query, k, collection |
| filtered_search | 带过滤搜索 | query, filter, k, collection |
| add_documents | 批量添加 | texts, metadatas, collection |
| get_collection_info | 集合信息 | collection |
| list_collections | 列出集合 | - |
| delete_document | 删除文档 | id, collection |

---

## 8.3 核心实现

```python
# MCP 工具定义 (JSON Schema 格式)
MCP_TOOLS = [
    {
        "name": "vector_search",
        "description": "Search LumenDB for semantically similar documents",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "k": {"type": "integer", "default": 10},
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["query"],
        },
    },
]

# 工具调用处理
async def call_tool(name: str, arguments: dict):
    if name == "vector_search":
        result = await tools.vector_search(
            arguments["query"],
            arguments.get("k", 10),
            arguments.get("collection", "default"),
        )
        return [TextContent(type="text", text=result)]
```

---

## 8.4 集成示例

```python
# LangChain 集成
from langchain.agents import AgentExecutor, create_openai_functions_agent
from langchain_community.tools import Tool

tools = [
    Tool(
        name="vector_search",
        func=lambda q: call_mcp("vector_search", {"query": q}),
        description="Search documents",
    )
]

agent = create_openai_functions_agent(llm, tools, prompt)
agent_executor = AgentExecutor(agent=agent, tools=tools)
```

---

## 思考题

1. MCP 的 stdio 传输模式和 SSE 传输模式各有什么适用场景？
2. 如果 MCP Server 返回错误，Agent 框架应该怎么处理？
3. MCP 工具的参数校验应该由谁做？Server 端还是 Client 端？

## 动手练习

1. 启动 MCP Server，用 `mcp` CLI 工具调用 `vector_search`
2. 添加一个新的 MCP 工具 `batch_search`，支持一次搜索多个 query
3. 在 LangChain 中创建一个使用 LumenDB MCP 工具的 Agent
