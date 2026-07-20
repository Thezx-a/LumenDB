"""
简单 HTTP 服务器 (回退方案) / Simple HTTP Server (Fallback).

当 FastAPI/uvicorn 不可用时, 使用 Python asyncio 标准库提供 HTTP 服务。
适合最小依赖的生产部署或边缘环境。

设计权衡 / Design Trade-offs:
  - 优点: 零依赖, 仅 Python 标准库 / Zero dependencies
  - 缺点: 功能有限, 无自动 OpenAPI 文档 / Limited features
  - 适用场景: 资源受限环境、快速测试 / Resource-constrained envs
"""

import asyncio
import json
import logging
from typing import Optional

from ..config import AgenticDBConfig
from ..llm.router import LLMRouter
from ..engine.multi_round import MultiRoundEngine
from .app import handle_query, handle_ask, handle_plan

logger = logging.getLogger(__name__)


async def handle_http_request(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
    engine: MultiRoundEngine,
    config: AgenticDBConfig,
):
    """
    处理单个 HTTP 请求 / Handle a single HTTP connection.

    解析 HTTP/1.1 请求, 路由到对应的处理函数, 返回 JSON 响应。

    Args:
        reader: asyncio 流读取器 / Stream reader
        writer: asyncio 流写入器 / Stream writer
        engine: 多轮检索引擎实例 / Multi-round engine
        config: 全局配置 / Global configuration
    """
    try:
        # 读取 HTTP 请求头 / Read HTTP request headers
        request_data = b""
        while True:
            chunk = await reader.read(4096)
            if not chunk:
                break
            request_data += chunk
            # HTTP 头部以 \r\n\r\n 结束 / Headers end with blank line
            if b"\r\n\r\n" in request_data:
                break

        if not request_data:
            writer.close()
            return

        request_str = request_data.decode("utf-8", errors="replace")
        lines = request_str.split("\r\n")
        if not lines:
            writer.close()
            return

        # 解析请求行 / Parse request line (e.g., "POST /query HTTP/1.1")
        request_line = lines[0]
        parts = request_line.split(" ")
        if len(parts) < 2:
            writer.close()
            return

        method = parts[0]
        path = parts[1]

        # 解析请求头 / Parse headers
        headers = {}
        for line in lines[1:]:
            if ": " in line:
                key, value = line.split(": ", 1)
                headers[key.lower()] = value

        # 解析请求体 / Parse body (if Content-Length header present)
        body_str = ""
        if "content-length" in headers:
            content_length = int(headers["content-length"])
            header_end = request_str.find("\r\n\r\n") + 4
            body_str = request_str[header_end: header_end + content_length]

        body = {}
        if body_str:
            try:
                body = json.loads(body_str)
            except json.JSONDecodeError:
                pass

        # 路由分发 / Route to handler
        resp_data = {"error": "not found"}
        status = 200

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
                status = 404
        except Exception as e:
            resp_data = {"error": str(e)}
            status = 500

        # 构建 HTTP 响应 / Build HTTP response
        resp_body = json.dumps(resp_data)
        resp_headers = (
            f"HTTP/1.1 {status} {'OK' if status == 200 else 'Error'}\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Connection: close\r\n"
            f"\r\n"
        )

        writer.write(resp_headers.encode() + resp_body.encode())
        await writer.drain()

    except Exception as e:
        logger.exception("HTTP handler error")
    finally:
        try:
            writer.close()
        except Exception:
            pass


async def run_simple_server(config: AgenticDBConfig):
    """
    启动简单 HTTP 服务器 / Run the simple async HTTP server.

    使用 asyncio.start_server 创建 TCP 服务器,
    每来一个连接创建新的 HTTP handler 协程。

    Args:
        config: AgenticDB 全局配置
    """
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    # 创建 TCP 服务器 / Create TCP server
    server = await asyncio.start_server(
        lambda r, w: handle_http_request(r, w, engine, config),
        config.agent_host,
        config.agent_port,
    )

    addr = server.sockets[0].getsockname()
    logger.info(
        "AgenticDB agent server listening on %s:%s", addr[0], addr[1]
    )

    async with server:
        await server.serve_forever()
