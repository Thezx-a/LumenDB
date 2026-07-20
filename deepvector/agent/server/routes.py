"""
绠€鍗?HTTP 鏈嶅姟鍣?(鍥為€€鏂规) / Simple HTTP Server (Fallback).

褰?FastAPI/uvicorn 涓嶅彲鐢ㄦ椂, 浣跨敤 Python asyncio 鏍囧噯搴撴彁渚?HTTP 鏈嶅姟銆?閫傚悎鏈€灏忎緷璧栫殑鐢熶骇閮ㄧ讲鎴栬竟缂樼幆澧冦€?
璁捐鏉冭　 / Design Trade-offs:
  - 浼樼偣: 闆朵緷璧? 浠?Python 鏍囧噯搴?/ Zero dependencies
  - 缂虹偣: 鍔熻兘鏈夐檺, 鏃犺嚜鍔?OpenAPI 鏂囨。 / Limited features
  - 閫傜敤鍦烘櫙: 璧勬簮鍙楅檺鐜銆佸揩閫熸祴璇?/ Resource-constrained envs
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
    澶勭悊鍗曚釜 HTTP 璇锋眰 / Handle a single HTTP connection.

    瑙ｆ瀽 HTTP/1.1 璇锋眰, 璺敱鍒板搴旂殑澶勭悊鍑芥暟, 杩斿洖 JSON 鍝嶅簲銆?
    Args:
        reader: asyncio 娴佽鍙栧櫒 / Stream reader
        writer: asyncio 娴佸啓鍏ュ櫒 / Stream writer
        engine: 澶氳疆妫€绱㈠紩鎿庡疄渚?/ Multi-round engine
        config: 鍏ㄥ眬閰嶇疆 / Global configuration
    """
    try:
        # 璇诲彇 HTTP 璇锋眰澶?/ Read HTTP request headers
        request_data = b""
        while True:
            chunk = await reader.read(4096)
            if not chunk:
                break
            request_data += chunk
            # HTTP 澶撮儴浠?\r\n\r\n 缁撴潫 / Headers end with blank line
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

        # 瑙ｆ瀽璇锋眰琛?/ Parse request line (e.g., "POST /query HTTP/1.1")
        request_line = lines[0]
        parts = request_line.split(" ")
        if len(parts) < 2:
            writer.close()
            return

        method = parts[0]
        path = parts[1]

        # 瑙ｆ瀽璇锋眰澶?/ Parse headers
        headers = {}
        for line in lines[1:]:
            if ": " in line:
                key, value = line.split(": ", 1)
                headers[key.lower()] = value

        # Parse body (finish reading Content-Length if incomplete)
        body_bytes = b""
        if "content-length" in headers:
            content_length = int(headers["content-length"])
            header_end = request_data.find(b"\r\n\r\n") + 4
            body_bytes = request_data[header_end:]
            while len(body_bytes) < content_length:
                chunk = await reader.read(content_length - len(body_bytes))
                if not chunk:
                    break
                body_bytes += chunk
            body_bytes = body_bytes[:content_length]

        body = {}
        if body_bytes:
            try:
                body = json.loads(body_bytes.decode("utf-8"))
            except json.JSONDecodeError:
                pass

        # Route to handler
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

        # Build HTTP response (Content-Length in bytes)
        resp_body = json.dumps(resp_data).encode("utf-8")
        resp_headers = (
            f"HTTP/1.1 {status} {'OK' if status == 200 else 'Error'}\r\n"
            f"Content-Type: application/json\r\n"
            f"Content-Length: {len(resp_body)}\r\n"
            f"Connection: close\r\n"
            f"\r\n"
        )

        writer.write(resp_headers.encode("utf-8") + resp_body)
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
    鍚姩绠€鍗?HTTP 鏈嶅姟鍣?/ Run the simple async HTTP server.

    浣跨敤 asyncio.start_server 鍒涘缓 TCP 鏈嶅姟鍣?
    姣忔潵涓€涓繛鎺ュ垱寤烘柊鐨?HTTP handler 鍗忕▼銆?
    Args:
        config: AgenticDB 鍏ㄥ眬閰嶇疆
    """
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    # 鍒涘缓 TCP 鏈嶅姟鍣?/ Create TCP server
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
