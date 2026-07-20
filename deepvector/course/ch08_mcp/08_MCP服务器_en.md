# Chapter 8: MCP Server

> MCP (Model Context Protocol) 鈥?plug-and-play DeepVector for any Agent framework.

## Prerequisites

> 馃搸 **Reference**: [Python Environment](../prerequisites/02_Python鐜_en.md)

---

## Learning Objectives

- Understand MCP protocol fundamentals
- Master MCP Server implementation
- Learn to integrate with Agent frameworks via MCP

---

## 8.1 What is MCP?

MCP is the "USB standard" for the AI Agent ecosystem.

```
Traditional:
  Agent Framework 鈫?Adapter 鈫?DeepVector SDK 鈫?DeepVector
                      鈫?                One per framework

MCP Way:
  Agent Framework 鈫?MCP Client 鈫?MCP Server 鈫?DeepVector
                                    鈫?                          Write once, use everywhere
```

---

## 8.2 Available Tools

| Tool | Function | Parameters |
|------|----------|------------|
| vector_search | Semantic search | query, k, collection |
| filtered_search | Search with filters | query, filter, k, collection |
| add_documents | Batch add | texts, metadatas, collection |
| get_collection_info | Collection stats | collection |
| list_collections | List all | - |
| delete_document | Delete by ID | id, collection |

---

## 8.3 Integration Example

```python
# LangChain with MCP tools
from langchain.agents import AgentExecutor

mcp_tools = [
    Tool(name="vector_search", func=call_mcp_search, ...)
]

agent = create_openai_functions_agent(llm, mcp_tools, prompt)
executor = AgentExecutor(agent=agent, tools=mcp_tools)
```

---

## Review Questions

1. When to use MCP stdio transport vs. SSE transport?
2. How should Agent frameworks handle MCP Server errors?
3. Who should validate tool parameters 鈥?Server or Client side?

## Hands-on Exercises

1. Start MCP Server and call `vector_search` via the `mcp` CLI
2. Add a new `batch_search` MCP tool
3. Create a LangChain Agent using DeepVector MCP tools
