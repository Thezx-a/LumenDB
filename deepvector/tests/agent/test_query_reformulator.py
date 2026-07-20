"""Tests for the QueryReformulator."""

import asyncio
import json
import pytest
from unittest.mock import AsyncMock

from agent.llm.router import LLMRouter, LLMResponse
from agent.engine.query_reformulator import QueryReformulator


def mock_llm_response(content: str):
    return LLMResponse(
        content=content,
        tool_calls=[],
        usage={},
        latency_ms=100.0,
        model="test",
        provider="test",
    )


@pytest.mark.asyncio
async def test_reformulate_generates_new_queries():
    """Test that reformulation generates new queries."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "new_queries": [
                    "HNSW graph structure nearest neighbor",
                    "approximate nearest neighbor search algorithm",
                ],
                "reasoning": "Using more specific technical terms",
            }
        )
    )

    reformulator = QueryReformulator(llm)
    queries = await reformulator.reformulate(
        question="How does HNSW work?",
        previous_queries=["HNSW explanation"],
        feedback="Results are too general, need more technical depth",
    )

    assert len(queries) == 2
    assert "HNSW graph structure" in queries[0]


@pytest.mark.asyncio
async def test_reformulate_returns_fallback_on_invalid_json():
    """Test fallback when LLM returns invalid JSON."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response("not valid json")

    reformulator = QueryReformulator(llm)
    queries = await reformulator.reformulate(
        question="Original question",
        previous_queries=["tried query"],
        feedback="needs improvement",
    )

    # Should return fallback with the original question
    assert len(queries) == 1
    assert queries[0] == "Original question"


@pytest.mark.asyncio
async def test_reformulate_includes_context():
    """Test that reformulation includes previous queries in context."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps({"new_queries": ["new query"], "reasoning": "test"})
    )

    reformulator = QueryReformulator(llm)
    await reformulator.reformulate(
        question="Test question",
        previous_queries=["query1", "query2"],
        feedback="Not enough results",
    )

    # Verify the LLM was called with proper context
    call_args = llm.chat.call_args
    messages = call_args[0][0]
    user_msg = messages[-1]["content"]

    assert "query1" in user_msg
    assert "query2" in user_msg
    assert "Not enough results" in user_msg


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
