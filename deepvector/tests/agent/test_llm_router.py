"""Tests for the LLM Router."""

import asyncio
import json
import pytest
from unittest.mock import AsyncMock, patch

from agent.config import LLMConfig
from agent.llm.router import LLMRouter, LLMResponse


@pytest.mark.asyncio
async def test_router_unknown_provider():
    """Test that unknown provider raises error."""
    config = LLMConfig(provider="unknown")
    router = LLMRouter(config)

    with pytest.raises(ValueError, match="Unknown LLM provider"):
        await router.chat([{"role": "user", "content": "test"}])


@pytest.mark.asyncio
async def test_router_initializes_client_lazily():
    """Test that HTTP client is created on first use."""
    config = LLMConfig(provider="ollama", ollama_host="http://localhost:11434")
    router = LLMRouter(config)

    assert router._client is None


def test_config_defaults():
    """Test that config has sensible defaults."""
    config = LLMConfig()

    assert config.provider == "ollama"
    assert config.model == "qwen2.5:7b"
    assert config.temperature == 0.1
    assert config.max_tokens == 2048


def test_llm_response_dataclass():
    """Test LLMResponse dataclass."""
    response = LLMResponse(
        content="hello",
        tool_calls=[{"id": "1", "name": "test", "arguments": {}}],
        usage={"tokens": 10},
        latency_ms=100.0,
        model="test-model",
        provider="test",
    )

    assert response.content == "hello"
    assert len(response.tool_calls) == 1
    assert response.latency_ms == 100.0


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
