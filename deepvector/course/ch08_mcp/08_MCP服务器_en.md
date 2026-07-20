# Chapter 8: MCP Server

> MCP (Model Context Protocol) — plug-and-play DeepVector for any Agent framework.

## Prerequisites

> 📎 **Reference**: [Python Environment](../prerequisites/02_Python环境_en.md)

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
  Agent Framework → Adapter → DeepVector SDK → DeepVector
                      ↑
                One per framework

MCP Way:
  Agent Framework → MCP Client → MCP Server → DeepVector
                                    ↑
                          Write once, use everywhere
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

## 8.3 Data flow in this repo (point → line → surface)

**Point:** MCP describes tools with JSON Schema; stdio/SSE carry JSON-RPC messages.

**Line:** Full `vector_search` path:

```
user query → MCP call_tool → EmbeddingService.encode → POST /search (float32)
          → DeepVector HNSW → ids + scores + meta
```

See `deepvector/agent/mcp/server.py`: embedding runs **in the Agent**, not in C++. The `collection` parameter is forwarded to the HTTP body and resolved by **CollectionRegistry**.

**Surface:** MCP + Multi-Round + LLM Router form the RAG stack:

| Component | File | Role |
|-----------|------|------|
| MCP Server | `agent/mcp/server.py` | Exposes 6 tools |
| Multi-Round | `agent/engine/multi_round.py` | Multi-hop retrieval + evaluation |
| LLM Router | `agent/llm/router.py` | Plan / reformulate / evaluate (mockable) |

```bash
cd deepvector && py -3 -m uvicorn agent.server.app:app --host 0.0.0.0 --port 8000
```

### Interview prompts

1. Where does MCP end and OpenAI Function Calling begin?
2. Why embed in Agent instead of the DB process?
3. How does the filter AST map to C++ `Filter`?

---

## 8.4 Integration Example

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
3. Who should validate tool parameters — Server or Client side?

## Hands-on Exercises

1. Start MCP Server and call `vector_search` via the `mcp` CLI
2. Add a new `batch_search` MCP tool
3. Create a LangChain Agent using DeepVector MCP tools

---

## Appendix: Interview Bank Mapping

After this chapter, drill the matching section in [INTERVIEW_BANK.md](../INTERVIEW_BANK.md) and self-check against [_CHAPTER_TEMPLATE.md](../_CHAPTER_TEMPLATE.md).

**Architecture:** [ARCHITECTURE.md](../../ARCHITECTURE.md) · **Tech:** [TECH.md](../../../TECH.md) · **Run:** [RUN.md](../../../RUN.md)
