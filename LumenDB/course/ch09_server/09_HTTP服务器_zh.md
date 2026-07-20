# 第九章：HTTP 服务器

> Agent Server — FastAPI 和简单 HTTP 双模式。

## 前置知识

> 📎 **参考**: [Python环境](../prerequisites/02_Python环境_zh.md) | [配置系统](../ch03_config/03_配置系统_zh.md)

---

## 学习目标

- 理解双模式服务器设计
- 掌握 API 端点设计
- 学会 FastAPI 和纯 Python 的选择策略

---

## 9.1 双模式架构

```mermaid
flowchart TD
    subgraph Client["客户端"]
        C1[curl]
        C2["Python SDK"]
        C3["浏览器"]
    end
    
    subgraph Server["Agent Server"]
        F[FastAPI (推荐)]
        S[Simple HTTP (回退)]
    end
    
    Client --> F
    Client --> S
    
    F --> LLM[LLM Router]
    S --> LLM
    
    LLM --> Engine[MultiRound Engine]
    Engine --> LDB[LumenDB HTTP API]
```

两种模式的选择：

| 模式 | 依赖 | 适用场景 | 功能 |
|------|------|----------|------|
| FastAPI | fastapi + uvicorn | 生产部署 | OpenAPI 文档, 自动校验, WebSocket |
| Simple HTTP | 无额外依赖 | 最小化部署, 嵌入式 | 基础路由, JSON 响应 |

---

## 9.2 API 端点

| 端点 | 方法 | 说明 |
|------|------|------|
| `/health` | GET | 健康检查 |
| `/query` | POST | 完整检索 (规划+执行+回答) |
| `/ask` | POST | 简洁问答 |
| `/plan` | POST | 仅生成检索计划 |

`/query` 请求示例:

```json
{
    "question": "对比 HNSW 和 IVF 的优缺点",
    "collection": "default",
    "max_rounds": 3
}
```

`/query` 响应示例:

```json
{
    "answer": "HNSW 通过分层图结构实现...",
    "documents": [
        {"id": 1, "distance": 0.12, "text": "HNSW search..."}
    ],
    "strategy": "multi_query",
    "rounds": 2,
    "quality_score": 0.85
}
```

---

## 9.3 错误处理

```python
try:
    if path == "/query" and method == "POST":
        resp_data = await handle_query(engine, body)
    else:
        resp_data = {"error": "not found"}
        status = 404
except Exception as e:
    resp_data = {"error": str(e)}
    status = 500
```

所有异常都会被捕获并返回 JSON 格式的错误消息，不会暴露内部堆栈。

---

## 思考题

1. FastAPI 的自动请求校验 (Pydantic) 相比手动解析有什么优势？
2. 如果请求 body 中的 `question` 长度超过 10000 字符，应该怎么处理？
3. 如何给 Agent Server 添加请求限流 (Rate Limiting)？

## 动手练习

1. 在 Simple HTTP 模式中增加 CORS 头支持
2. 给 `/query` 添加 `stream` 参数，实现 SSE 流式返回
3. 实现一个 `/batch/query` 端点，一次处理多个问题
