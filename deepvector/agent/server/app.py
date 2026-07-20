"""
AgenticDB Agent HTTP Server / Agent HTTP 鏈嶅姟鍣?

鎻愪緵 /query, /ask, /plan 绔偣渚涘鎴风璋冪敤銆?鏀寔 FastAPI (鎺ㄨ崘) 鍜岀函 Python HTTP 涓ょ妯″紡銆?
绔偣璇勫垎 / API Endpoints:
  GET  /health  鈥?鍋ュ悍妫€鏌?/ Health check
  POST /query   鈥?瀹屾暣妫€绱?(瑙勫垝+鎵ц+鍥炵瓟) / Full agent search
  POST /ask     鈥?绠€娲侀棶绛?/ Simple Q&A
  POST /plan    鈥?浠呯敓鎴愭绱㈣鍒?/ Just generate plan

閮ㄧ讲鎷撴墤 / Deployment Topology:
  鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?    HTTP (8080)     鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?Python Agent   鈹?鈼勨攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻?鈹?DeepVector C++   鈹?  鈹?Server (8090)  鈹?                     鈹?Server (8080) 鈹?  鈹?               鈹?                     鈹?               鈹?  鈹?/query 鈫?LLM   鈹?                     鈹?/search        鈹?  鈹?/ask   鈫?LLM   鈹?                     鈹?/insert        鈹?  鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                     鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?          鈹?LLM API
          鈻?  鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?OpenAI API     鈹?  鈹?鎴?Ollama      鈹?  鈹?(localhost)    鈹?  鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?"""

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

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
    澶勭悊 /query 璇锋眰 鈥?瀹屾暣鐨?Agentic Search / Handle /query endpoint.

    杩斿洖鍖呭惈妫€绱㈢瓥鐣ャ€佸杞繃绋嬨€佽川閲忚瘎鍒嗗拰鏈€缁堢瓟妗堢殑瀹屾暣缁撴灉銆?
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

    engine.config.max_rounds = max_rounds
    try:
        result = await engine.retrieve(question, collection)
        return result.to_dict()
    except Exception as e:
        logger.exception("Query failed")
        return {"error": str(e)}
    finally:
        engine.config.max_rounds = 5


async def handle_ask(
    engine: MultiRoundEngine, body: Dict[str, Any]
) -> Dict[str, Any]:
    """
    澶勭悊 /ask 璇锋眰 鈥?绠€娲侀棶绛?/ Handle /ask endpoint 鈥?simple Q&A.

    杩斿洖绠€鍖栫殑绛旀鍜屽厓淇℃伅, 閫傚悎绠€鍗曟煡璇€?
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
    澶勭悊 /plan 璇锋眰 鈥?浠呯敓鎴愯鍒? 涓嶆墽琛?/ Handle /plan endpoint.

    鐢ㄤ簬璋冭瘯鍜屽睍绀?LLM 鐨勮鍒掕兘鍔涖€?
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
    鍒涘缓 FastAPI 搴旂敤 / Create FastAPI application.

    闇€瑕佸畨瑁?fastapi 鍜?uvicorn:
        pip install fastapi uvicorn

    Returns:
        FastAPI 搴旂敤瀹炰緥, 鍙洿鎺?uvicorn.run()
    """
    from fastapi import FastAPI
    from pydantic import BaseModel

    config = load_config()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    app = FastAPI(title="AgenticDB", version="0.1.0")

    class QueryRequest(BaseModel):
        """璇锋眰浣?Schema / Request body schema for /query and /ask."""
        question: str
        collection: str = "default"
        max_rounds: int = 5

    @app.get("/health")
    async def health():
        """鍋ュ悍妫€鏌?/ Health check endpoint."""
        return {
            "status": "ok",
            "model": config.llm.model,
            "provider": config.llm.provider,
        }

    @app.post("/query")
    async def query(req: QueryRequest):
        """瀹屾暣 agentic 鎼滅储 / Full agentic search with planning + multi-round."""
        return await handle_query(engine, req.dict())

    @app.post("/ask")
    async def ask(req: QueryRequest):
        """绠€娲侀棶绛?/ Simple Q&A with retrieval."""
        return await handle_ask(engine, req.dict())

    @app.post("/plan")
    async def plan(req: QueryRequest):
        """浠呯敓鎴愭绱㈣鍒?/ Generate retrieval plan only."""
        return await handle_plan(engine, req.dict())

    @app.on_event("shutdown")
    async def shutdown():
        """浼橀泤鍏抽棴 / Graceful shutdown 鈥?release resources."""
        await llm.close()
        await engine.close()

    return app


def create_simple_http_server():
    """
    鍒涘缓绠€鍗?HTTP 鏈嶅姟鍣?(鏃犻渶 FastAPI) / Create simple HTTP server (no FastAPI).

    鐢ㄤ簬鏈€灏忎緷璧栭儴缃? 浣跨敤 Python 鏍囧噯搴?asyncio 瀹炵幇銆?
    Returns:
        (ASGI app function, engine, llm) 鍏冪粍
    """
    import asyncio
    from urllib.parse import urlparse, parse_qs

    config = load_config()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    async def app_scope(scope, receive, send):
        """ASGI 搴旂敤 / ASGI application."""
        if scope["type"] != "http":
            return

        method = scope["method"]
        path = scope["path"]

        # 璇诲彇 HTTP body / Read HTTP body
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

        # 璺敱鍒嗗彂 / Route dispatching
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
    """鍏ュ彛: 鍚姩 AgenticDB Agent 鏈嶅姟鍣?/ Entry point: start Agent server."""
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
