"""
AgenticDB Agent HTTP Server / Agent HTTP 服务器.

提供 /query, /ask, /plan 端点供客户端调用。
支持 FastAPI (推荐) 和纯 Python HTTP 两种模式。

端点评分 / API Endpoints:
  GET  /health  — 健康检查 / Health check
  POST /query   — 完整检索 (规划+执行+回答) / Full agent search
  POST /ask     — 简洁问答 / Simple Q&A
  POST /plan    — 仅生成检索计划 / Just generate plan

部署拓扑 / Deployment Topology:
  ┌────────────────┐     HTTP (8080)     ┌────────────────┐
  │ Python Agent   │ ◄──────────────────► │ DeepVector C++   │
  │ Server (8090)  │                      │ Server (8080) │
  │                │                      │                │
  │ /query → LLM   │                      │ /search        │
  │ /ask   → LLM   │                      │ /insert        │
  └───────┬────────┘                      └────────────────┘
          │ LLM API
          ▼
  ┌────────────────┐
  │ OpenAI API     │
  │ 或 Ollama      │
  │ (localhost)    │
  └────────────────┘
"""

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import json
import logging
from typing import Any, Dict

from agent.config import load_config
from agent.llm.router import LLMRouter
from agent.engine.multi_round import MultiRoundEngine

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)


async def handle_query(
    engine: MultiRoundEngine, body: Dict[str, Any]
) -> Dict[str, Any]:
    """
    处理 /query 请求 — 完整的 Agentic Search / Handle /query endpoint.

    返回包含检索策略、多轮过程、质量评分和最终答案的完整结果。

    Request body:
        {"question": "...", "collection": "default", "max_rounds": 5}

    Response:
        {"answer": "...", "documents": [...], "strategy": "direct",
         "rounds": 1, "quality_score": 0.85, "queries_tried": [...]}
    """
    question = body.get("question", "")
    collection = body.get("collection", engine.config.default_collection)
    max_rounds = body.get("max_rounds", engine.config.max_rounds)

    if not question:
        return {"error": "question is required"}

    prev_rounds = engine.config.max_rounds
    engine.config.max_rounds = max_rounds
    try:
        result = await engine.retrieve(question, collection)
        return result.to_dict()
    except Exception as e:
        logger.exception("Query failed")
        return {"error": str(e)}
    finally:
        engine.config.max_rounds = prev_rounds


async def handle_ask(
    engine: MultiRoundEngine, body: Dict[str, Any]
) -> Dict[str, Any]:
    """
    处理 /ask 请求 — 简洁问答 / Handle /ask endpoint — simple Q&A.

    返回简化的答案和元信息, 适合简单查询。

    Request body:
        {"question": "..."}

    Response:
        {"answer": "...", "documents": 5, "rounds": 1, "quality": 0.85}
    """
    question = body.get("question", "")
    if not question:
        return {"error": "question is required"}

    try:
        result = await engine.retrieve(question)
        return {
            "answer": result.answer,
            "documents": len(result.documents),
            "rounds": result.rounds,
            "quality": result.quality_score,
        }
    except Exception as e:
        logger.exception("Ask failed")
        return {"error": str(e)}


async def handle_plan(
    engine: MultiRoundEngine, body: Dict[str, Any]
) -> Dict[str, Any]:
    """
    处理 /plan 请求 — 仅生成计划, 不执行 / Handle /plan endpoint.

    用于调试和展示 LLM 的规划能力。

    Request body:
        {"question": "..."}

    Response:
        {"strategy": "direct", "reasoning": "...", "steps": 1}
    """
    question = body.get("question", "")
    collection = body.get("collection", engine.config.default_collection)
    if not question:
        return {"error": "question is required"}

    try:
        plan = await engine.planner.plan(question, collection)
        return {
            "strategy": plan.strategy.value,
            "reasoning": plan.reasoning,
            "steps": len(plan.steps),
        }
    except Exception as e:
        return {"error": str(e)}


def create_fastapi_app():
    """
    Create FastAPI application (Pydantic v2 + lifespan API).

    Install: pip install fastapi uvicorn
    """
    from contextlib import asynccontextmanager

    from fastapi import FastAPI
    from pydantic import BaseModel, Field

    config = load_config()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    @asynccontextmanager
    async def lifespan(_app: FastAPI):
        yield
        await llm.close()
        await engine.close()

    app = FastAPI(title="AgenticDB", version="0.1.0", lifespan=lifespan)

    @app.middleware("http")
    async def add_request_id(request, call_next):
        import uuid
        rid = request.headers.get("x-request-id") or str(uuid.uuid4())
        response = await call_next(request)
        response.headers["X-Request-Id"] = rid
        return response

    class QueryRequest(BaseModel):
        question: str
        collection: str = "default"
        max_rounds: int = Field(default=5, ge=1, le=20)

    @app.get("/health")
    async def health():
        return {
            "status": "ok",
            "model": config.llm.model,
            "provider": config.llm.provider,
        }

    @app.post("/query")
    async def query(req: QueryRequest):
        return await handle_query(engine, req.model_dump())

    @app.post("/ask")
    async def ask(req: QueryRequest):
        return await handle_ask(engine, req.model_dump())

    @app.post("/plan")
    async def plan(req: QueryRequest):
        return await handle_plan(engine, req.model_dump())

    return app


create_app = create_fastapi_app


def create_simple_http_server():
    """
    创建简单 HTTP 服务器 (无需 FastAPI) / Create simple HTTP server (no FastAPI).

    用于最小依赖部署, 使用 Python 标准库 asyncio 实现。

    Returns:
        (ASGI app function, engine, llm) 元组
    """
    import asyncio
    from urllib.parse import urlparse, parse_qs

    config = load_config()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    async def app_scope(scope, receive, send):
        """ASGI 应用 / ASGI application."""
        if scope["type"] != "http":
            return

        method = scope["method"]
        path = scope["path"]

        # 读取 HTTP body / Read HTTP body
        body_bytes = b""
        more_body = True
        while more_body:
            message = await receive()
            if message["type"] == "http.request":
                body_bytes += message.get("body", b"")
                more_body = message.get("more_body", False)

        body_str = body_bytes.decode("utf-8", errors="replace")
        body = {}
        if body_str:
            try:
                body = json.loads(body_str)
            except json.JSONDecodeError:
                pass

        # 路由分发 / Route dispatching
        try:
            if path == "/health" and method == "GET":
                resp_data = {
                    "status": "ok",
                    "model": config.llm.model,
                    "provider": config.llm.provider,
                }
            elif path == "/query" and method == "POST":
                resp_data = await handle_query(engine, body)
            elif path == "/ask" and method == "POST":
                resp_data = await handle_ask(engine, body)
            elif path == "/plan" and method == "POST":
                resp_data = await handle_plan(engine, body)
            else:
                resp_data = {"error": "not found"}
        except Exception as e:
            resp_data = {"error": str(e)}

        resp_body = json.dumps(resp_data).encode("utf-8")
        await send({
            "type": "http.response.start",
            "status": 200,
            "headers": [
                (b"content-type", b"application/json"),
                (b"content-length", str(len(resp_body)).encode()),
            ],
        })
        await send({"type": "http.response.body", "body": resp_body})

    return app_scope, engine, llm


if __name__ == "__main__":
    """入口: 启动 AgenticDB Agent 服务器 / Entry point: start Agent server."""
    try:
        import uvicorn

        config = load_config()
        app = create_fastapi_app()
        logger.info(
            "Starting AgenticDB agent server on %s:%d",
            config.agent_host,
            config.agent_port,
        )
        uvicorn.run(app, host=config.agent_host, port=config.agent_port)
    except ImportError:
        logger.warning(
            "FastAPI/uvicorn not installed. "
            "Install with: pip install fastapi uvicorn"
        )
        logger.info("Falling back to simple HTTP server...")
        import asyncio
        from agent.server.routes import run_simple_server

        config = load_config()
        asyncio.run(run_simple_server(config))
