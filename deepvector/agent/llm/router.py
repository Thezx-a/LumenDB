"""
LLM Router 鈥?OpenAI 涓?Ollama 鐨勭粺涓€寮傛鎺ュ彛 / Unified async interface for OpenAI and Ollama.

鏍稿績璁捐 / Design:
  - 绛栫暐妯″紡: 鏍规嵁 config.provider 鑷姩閫夋嫨鍚庣 / Strategy pattern for provider selection
  - 寮傛 HTTP: 浣跨敤 httpx.AsyncClient 閬垮厤闃诲浜嬩欢寰幆 / Non-blocking IO
  - 寤惰繜鍒濆鍖? HTTP 杩炴帴鍦ㄩ娆¤皟鐢ㄦ椂鍒涘缓, 閬垮厤绌哄惎鍔?/ Lazy client init
  - 缁熶竴杩斿洖: LLMResponse 灏佽 content/tool_calls/latency 绛?/ Unified response format

渚涘簲鍟嗗樊寮?/ Provider Differences:
  - OpenAI API: 鍘熺敓 tool_calls 鏀寔, id 涓?call_xxx 鏍煎紡
  - Ollama API: tool_calls 鍦?message 涓? id 澶嶇敤 function name
  - Ollama 鐨?token 缁熻瀛楁鍚嶄笉鍚?(prompt_eval_count vs usage.prompt_tokens)
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
    LLM 璋冪敤缁撴灉 / LLM invocation result.

    Attributes:
        content: 鏂囨湰鍥炲鍐呭 / Text response content
        tool_calls: 宸ュ叿璋冪敤鍒楄〃, 姣忎釜鍖呭惈 id/name/arguments (宸茶В鏋愪负 dict)
        usage: Token 鐢ㄩ噺缁熻 / Token usage stats
        latency_ms: 绔埌绔欢杩?(姣) / End-to-end latency in ms
        model: 浣跨敤鐨勬ā鍨嬪悕绉?/ Model name used
        provider: 渚涘簲鍟嗗悕绉?/ Provider name ("openai" | "ollama")
    """
    content: str = ""
    tool_calls: List[Dict[str, Any]] = field(default_factory=list)
    usage: Dict[str, int] = field(default_factory=dict)
    latency_ms: float = 0.0
    model: str = ""
    provider: str = ""


class LLMRouter:
    """
    LLM 璺敱鍣?/ LLM Router 鈥?缁熶竴鎺ュ彛, 澶氬悗绔敮鎸?

    鐢ㄦ硶 / Usage:
        config = LLMConfig(provider="ollama", model="qwen2.5:7b")
        router = LLMRouter(config)

        # 绠€鍗曞璇?/ Simple chat
        response = await router.chat([
            {"role": "user", "content": "What is RAG?"}
        ])
        print(response.content)

        # 甯﹀伐鍏疯皟鐢?/ With tool calling
        response = await router.chat(messages, tools=[SEARCH_TOOL])
        for tc in response.tool_calls:
            print(f"Call {tc['name']} with {tc['arguments']}")
    """

    def __init__(self, config: LLMConfig):
        """
        鍒濆鍖?LLM 璺敱鍣?/ Initialize LLM router.

        Args:
            config: LLM 閰嶇疆, 鍖呭惈 provider/model/keys 绛?        """
        self.config = config
        # 寤惰繜鍒涘缓: 棣栨 chat() 鏃舵墠鍒濆鍖?HTTP 瀹㈡埛绔?/ Lazy HTTP client creation
        self._client: Optional[httpx.AsyncClient] = None

    async def _ensure_client(self):
        """
        纭繚 HTTP 瀹㈡埛绔凡鍒濆鍖?/ Ensure HTTP client is initialized.

        浣跨敤 httpx.AsyncClient 鏀寔杩炴帴姹犲鐢ㄥ拰瓒呮椂鎺у埗銆?        """
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=self.config.timeout)

    async def close(self):
        """
        鍏抽棴 HTTP 瀹㈡埛绔? 閲婃斁杩炴帴 / Close HTTP client and release connections.

        搴斿湪搴旂敤鍏抽棴鏃惰皟鐢? 閬垮厤杩炴帴娉勬紡銆?        """
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
        鍙戦€佽亰澶╄ˉ鍏ㄨ姹?/ Send chat completion request.

        鏍规嵁閰嶇疆鑷姩閫夋嫨 OpenAI API 鎴?Ollama API銆?        鑷姩璁板綍绔埌绔欢杩熺敤浜庢€ц兘鐩戞帶銆?
        Args:
            messages: 瀵硅瘽娑堟伅鍒楄〃, 鏍煎紡: [{"role": "system"|"user"|"assistant", "content": "..."}]
            tools: 宸ュ叿瀹氫箟鍒楄〃 (OpenAI function calling 鏍煎紡)
            temperature: 瑕嗙洊榛樿娓╁害 / Override default temperature
            max_tokens: 瑕嗙洊鏈€澶?token 鏁?/ Override max output tokens

        Returns:
            LLMResponse 缁熶竴鍝嶅簲瀵硅薄

        Raises:
            ValueError: 鏈煡鎻愪緵鍟?/ Unknown provider
            httpx.HTTPError: HTTP 璇锋眰澶辫触 / HTTP request failure
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
        OpenAI API 瀹炵幇 / OpenAI API implementation.

        绔偣: POST /v1/chat/completions
        鏀寔 tools/tool_choice 鍙傛暟瀹炵幇 function calling銆?
        Returns:
            LLMResponse 鍖呭惈 content銆乼ool_calls銆乽sage 鍜屽欢杩熺粺璁?        """
        # 鏋勫缓璇锋眰 URL 鍜屽ご閮?/ Build request URL and headers
        base_url = self.config.openai_base_url or "https://api.openai.com/v1"
        url = f"{base_url}/chat/completions"
        headers = {"Authorization": f"Bearer {self.config.openai_api_key}"}

        # 鏋勫缓璇锋眰浣?/ Build request payload
        payload: Dict[str, Any] = {
            "model": self.config.model,
            "messages": messages,
            "temperature": temperature,
            "max_tokens": max_tokens,
        }
        if tools:
            payload["tools"] = tools
            payload["tool_choice"] = "auto"  # 璁╂ā鍨嬭嚜涓诲喅瀹氭槸鍚﹁皟鐢ㄥ伐鍏?
        # 鍙戦€佽姹傚苟璁板綍寤惰繜 / Send request with latency tracking
        start = time.monotonic()
        resp = await self._client.post(url, json=payload, headers=headers)
        latency = (time.monotonic() - start) * 1000
        resp.raise_for_status()
        data = resp.json()

        # 瑙ｆ瀽鍝嶅簲 / Parse response
        choice = data["choices"][0]
        message = choice["message"]
        content = message.get("content", "") or ""

        # 瑙ｆ瀽宸ュ叿璋冪敤 (濡傛灉鏈? / Parse tool calls if present
        tool_calls = []
        if message.get("tool_calls"):
            for tc in message["tool_calls"]:
                tool_calls.append({
                    "id": tc["id"],                                    # call_xxx 鏍煎紡
                    "name": tc["function"]["name"],                     # 鍑芥暟鍚嶇О
                    "arguments": json.loads(tc["function"]["arguments"]),  # 宸茶В鏋愪负 dict
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
        Ollama API 瀹炵幇 / Ollama API implementation.

        绔偣: POST /api/chat
        Ollama 鐨?tool_calls 鏍煎紡涓?OpenAI 鐣ユ湁涓嶅悓:
          - Ollama: message.tool_calls[i].function.arguments 涓?dict
          - OpenAI: message.tool_calls[i].function.arguments 涓?string (JSON)

        Returns:
            LLMResponse 缁熶竴鍝嶅簲瀵硅薄
        """
        url = f"{self.config.ollama_host}/api/chat"

        # Ollama 鐨?options 瀛楁灏佽妯″瀷鍙傛暟
        payload: Dict[str, Any] = {
            "model": self.config.model,
            "messages": messages,
            "stream": False,  # 绂佺敤娴佸紡鍝嶅簲 / Disable streaming
            "options": {
                "temperature": temperature,
                "num_predict": max_tokens,
            },
        }
        if tools:
            payload["tools"] = tools

        # 鍙戦€佽姹傚苟璁板綍寤惰繜 / Send request with latency tracking
        start = time.monotonic()
        resp = await self._client.post(url, json=payload)
        latency = (time.monotonic() - start) * 1000
        resp.raise_for_status()
        data = resp.json()

        # 瑙ｆ瀽鍝嶅簲 / Parse response
        message = data.get("message", {})
        content = message.get("content", "") or ""

        # 瑙ｆ瀽宸ュ叿璋冪敤 / Parse Ollama tool calls
        tool_calls = []
        if message.get("tool_calls"):
            for tc in message["tool_calls"]:
                func = tc.get("function", {})
                tool_calls.append({
                    "id": func.get("name", ""),           # Ollama 澶嶇敤 function name 浣滀负 id
                    "name": func.get("name", ""),
                    "arguments": func.get("arguments", {}),  # Ollama 鐩存帴杩斿洖 dict
                })

        # Ollama 鐨?token 缁熻瀛楁鍚嶄笉鍚?/ Different field names for Ollama
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
