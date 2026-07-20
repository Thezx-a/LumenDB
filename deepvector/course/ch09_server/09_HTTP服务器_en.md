# Chapter 9: HTTP Server

> Agent Server — dual-mode: FastAPI and simple HTTP.

## Prerequisites

> 📎 **Reference**: [Python Environment](../prerequisites/02_Python环境_en.md) | [Configuration](../ch03_config/03_配置系统_en.md)

---

## Learning Objectives

- Understand the dual-mode server design
- Master API endpoint design
- Learn FastAPI vs. pure Python trade-offs

---

## 9.1 Dual-Mode Architecture

| Mode | Dependencies | Use Case | Features |
|------|-------------|----------|----------|
| FastAPI | fastapi + uvicorn | Production | OpenAPI docs, validation, WebSocket |
| Simple HTTP | None | Minimal deployment | Basic routing, JSON responses |

---

## 9.2 API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Health check |
| `/query` | POST | Full search (plan + execute + answer) |
| `/ask` | POST | Simple Q&A |
| `/plan` | POST | Generate plan only |

Example `/query` request:
```json
{
    "question": "Compare HNSW and IVF",
    "collection": "default",
    "max_rounds": 3
}
```

---

## 9.3 Error Handling

```python
try:
    resp_data = await handle_query(engine, body)
except Exception as e:
    resp_data = {"error": str(e)}
    status = 500
```

All exceptions caught and returned as JSON — no stack trace leakage.

---

## Review Questions

1. What advantages does FastAPI's Pydantic validation offer over manual parsing?
2. How should the server handle `question` fields over 10000 characters?
3. How to add rate limiting to the Agent Server?

## Hands-on Exercises

1. Add CORS headers to the Simple HTTP server
2. Add `stream` parameter to `/query` for SSE support
3. Implement `/batch/query` for processing multiple questions

---

## Appendix: Interview Bank Mapping

After this chapter, drill the matching section in [INTERVIEW_BANK.md](../INTERVIEW_BANK.md) and self-check against [_CHAPTER_TEMPLATE.md](../_CHAPTER_TEMPLATE.md).

**Architecture:** [ARCHITECTURE.md](../../ARCHITECTURE.md) · **Tech:** [TECH.md](../../../TECH.md) · **Run:** [RUN.md](../../../RUN.md)
