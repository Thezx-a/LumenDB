"""
LLM Router — OpenAI 与 Ollama 的统一异步接口 / Unified async interface for OpenAI and Ollama.

核心设计 / Design:
  - 策略模式: 根据 config.provider 自动选择后端 / Strategy pattern for provider selection
  - 异步 HTTP: 使用 httpx.AsyncClient 避免阻塞事件循环 / Non-blocking IO
  - 延迟初始化: HTTP 连接在首次调用时创建, 避免空启动 / Lazy client init
  - 统一返回: LLMResponse 封装 content/tool_calls/latency 等 / Unified response format

供应商差异 / Provider Differences:
  - OpenAI API: 原生 tool_calls 支持, id 为 call_xxx 格式
  - Ollama API: tool_calls 在 message 中, id 复用 function name
  - Ollama 的 token 统计字段名不同 (prompt_eval_count vs usage.prompt_tokens)
"""

import json
import time
import logging
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

import httpx

from ..config import LLMConfig

logger = logging.getLogger(__name__)


@dataclass
class LLMResponse:
    """
    LLM 调用结果 / LLM invocation result.

    Attributes:
        content: 文本回复内容 / Text response content
        tool_calls: 工具调用列表, 每个包含 id/name/arguments (已解析为 dict)
        usage: Token 用量统计 / Token usage stats
        latency_ms: 端到端延迟 (毫秒) / End-to-end latency in ms
        model: 使用的模型名称 / Model name used
        provider: 供应商名称 / Provider name ("openai" | "ollama")
    """
    content: str = ""
    tool_calls: List[Dict[str, Any]] = field(default_factory=list)
    usage: Dict[str, int] = field(default_factory=dict)
    latency_ms: float = 0.0
    model: str = ""
    provider: str = ""


class LLMRouter:
    """
    LLM 路由器 / LLM Router — 统一接口, 多后端支持.

    用法 / Usage:
        config = LLMConfig(provider="ollama", model="qwen2.5:7b")
        router = LLMRouter(config)

        # 简单对话 / Simple chat
        response = await router.chat([
            {"role": "user", "content": "What is RAG?"}
        ])
        print(response.content)

        # 带工具调用 / With tool calling
        response = await router.chat(messages, tools=[SEARCH_TOOL])
        for tc in response.tool_calls:
            print(f"Call {tc['name']} with {tc['arguments']}")
    """

    def __init__(self, config: LLMConfig):
        """
        初始化 LLM 路由器 / Initialize LLM router.

        Args:
            config: LLM 配置, 包含 provider/model/keys 等
        """
        self.config = config
        # 延迟创建: 首次 chat() 时才初始化 HTTP 客户端 / Lazy HTTP client creation
        self._client: Optional[httpx.AsyncClient] = None

    async def _ensure_client(self):
        """
        确保 HTTP 客户端已初始化 / Ensure HTTP client is initialized.

        使用 httpx.AsyncClient 支持连接池复用和超时控制。
        """
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=self.config.timeout)

    async def close(self):
        """
        关闭 HTTP 客户端, 释放连接 / Close HTTP client and release connections.

        应在应用关闭时调用, 避免连接泄漏。
        """
        if self._client and not self._client.is_closed:
            await self._client.aclose()

    async def chat(
        self,
        messages: List[Dict[str, str]],
        tools: Optional[List[Dict[str, Any]]] = None,
        temperature: Optional[float] = None,
        max_tokens: Optional[int] = None,
    ) -> LLMResponse:
        """
        发送聊天补全请求 / Send chat completion request.

        根据配置自动选择 OpenAI API 或 Ollama API。
        自动记录端到端延迟用于性能监控。

        Args:
            messages: 对话消息列表, 格式: [{"role": "system"|"user"|"assistant", "content": "..."}]
            tools: 工具定义列表 (OpenAI function calling 格式)
            temperature: 覆盖默认温度 / Override default temperature
            max_tokens: 覆盖最大 token 数 / Override max output tokens

        Returns:
            LLMResponse 统一响应对象

        Raises:
            ValueError: 未知提供商 / Unknown provider
            httpx.HTTPError: HTTP 请求失败 / HTTP request failure
        """
        await self._ensure_client()
        temp = temperature if temperature is not None else self.config.temperature
        tokens = max_tokens if max_tokens is not None else self.config.max_tokens

        if self.config.provider == "openai":
            return await self._chat_openai(messages, tools, temp, tokens)
        elif self.config.provider == "ollama":
            return await self._chat_ollama(messages, tools, temp, tokens)
        else:
            raise ValueError(f"Unknown LLM provider: {self.config.provider}")

    async def _chat_openai(
        self,
        messages: List[Dict[str, str]],
        tools: Optional[List[Dict[str, Any]]],
        temperature: float,
        max_tokens: int,
    ) -> LLMResponse:
        """
        OpenAI API 实现 / OpenAI API implementation.

        端点: POST /v1/chat/completions
        支持 tools/tool_choice 参数实现 function calling。

        Returns:
            LLMResponse 包含 content、tool_calls、usage 和延迟统计
        """
        # 构建请求 URL 和头部 / Build request URL and headers
        base_url = self.config.openai_base_url or "https://api.openai.com/v1"
        url = f"{base_url}/chat/completions"
        headers = {"Authorization": f"Bearer {self.config.openai_api_key}"}

        # 构建请求体 / Build request payload
        payload: Dict[str, Any] = {
            "model": self.config.model,
            "messages": messages,
            "temperature": temperature,
            "max_tokens": max_tokens,
        }
        if tools:
            payload["tools"] = tools
            payload["tool_choice"] = "auto"  # 让模型自主决定是否调用工具

        # 发送请求并记录延迟 / Send request with latency tracking
        start = time.monotonic()
        resp = await self._client.post(url, json=payload, headers=headers)
        latency = (time.monotonic() - start) * 1000
        resp.raise_for_status()
        data = resp.json()

        # 解析响应 / Parse response
        choice = data["choices"][0]
        message = choice["message"]
        content = message.get("content", "") or ""

        # 解析工具调用 (如果有) / Parse tool calls if present
        tool_calls = []
        if message.get("tool_calls"):
            for tc in message["tool_calls"]:
                tool_calls.append({
                    "id": tc["id"],                                    # call_xxx 格式
                    "name": tc["function"]["name"],                     # 函数名称
                    "arguments": json.loads(tc["function"]["arguments"]),  # 已解析为 dict
                })

        return LLMResponse(
            content=content,
            tool_calls=tool_calls,
            usage=data.get("usage", {}),
            latency_ms=latency,
            model=self.config.model,
            provider="openai",
        )

    async def _chat_ollama(
        self,
        messages: List[Dict[str, str]],
        tools: Optional[List[Dict[str, Any]]],
        temperature: float,
        max_tokens: int,
    ) -> LLMResponse:
        """
        Ollama API 实现 / Ollama API implementation.

        端点: POST /api/chat
        Ollama 的 tool_calls 格式与 OpenAI 略有不同:
          - Ollama: message.tool_calls[i].function.arguments 为 dict
          - OpenAI: message.tool_calls[i].function.arguments 为 string (JSON)

        Returns:
            LLMResponse 统一响应对象
        """
        url = f"{self.config.ollama_host}/api/chat"

        # Ollama 的 options 字段封装模型参数
        payload: Dict[str, Any] = {
            "model": self.config.model,
            "messages": messages,
            "stream": False,  # 禁用流式响应 / Disable streaming
            "options": {
                "temperature": temperature,
                "num_predict": max_tokens,
            },
        }
        if tools:
            payload["tools"] = tools

        # 发送请求并记录延迟 / Send request with latency tracking
        start = time.monotonic()
        resp = await self._client.post(url, json=payload)
        latency = (time.monotonic() - start) * 1000
        resp.raise_for_status()
        data = resp.json()

        # 解析响应 / Parse response
        message = data.get("message", {})
        content = message.get("content", "") or ""

        # 解析工具调用 / Parse Ollama tool calls
        tool_calls = []
        if message.get("tool_calls"):
            for tc in message["tool_calls"]:
                func = tc.get("function", {})
                tool_calls.append({
                    "id": func.get("name", ""),           # Ollama 复用 function name 作为 id
                    "name": func.get("name", ""),
                    "arguments": func.get("arguments", {}),  # Ollama 直接返回 dict
                })

        # Ollama 的 token 统计字段名不同 / Different field names for Ollama
        return LLMResponse(
            content=content,
            tool_calls=tool_calls,
            usage={
                "prompt_tokens": data.get("prompt_eval_count", 0),
                "completion_tokens": data.get("eval_count", 0),
            },
            latency_ms=latency,
            model=self.config.model,
            provider="ollama",
        )
