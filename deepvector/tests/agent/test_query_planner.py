"""Tests for the QueryPlanner."""

import asyncio
import json
import pytest
from unittest.mock import AsyncMock, MagicMock

from agent.config import AgenticDBConfig
from agent.llm.router import LLMRouter, LLMResponse
from agent.engine.query_planner import QueryPlanner
from agent.engine.strategy import SearchStrategy


def make_config():
    return AgenticDBConfig()


def mock_llm_response(content: str, tool_calls=None):
    return LLMResponse(
        content=content,
        tool_calls=tool_calls or [],
        usage={},
        latency_ms=100.0,
        model="test",
        provider="test",
    )


@pytest.mark.asyncio
async def test_plan_direct_strategy():
    """Test that a simple question gets DIRECT strategy."""
    config = make_config()
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "strategy": "DIRECT",
                "reasoning": "Simple factual question",
                "searches": [{"query": "What is RAG?", "k": 10}],
                "expected_rounds": 1,
            }
        )
    )

    planner = QueryPlanner(llm, "default")
    plan = await planner.plan("What is RAG?")

    assert plan.strategy == SearchStrategy.DIRECT
    assert len(plan.steps) == 1
    assert plan.steps[0].query == "What is RAG?"
    assert plan.steps[0].k == 10
    assert plan.original_question == "What is RAG?"


@pytest.mark.asyncio
async def test_plan_multi_query_strategy():
    """Test that a complex question gets MULTI_QUERY strategy."""
    config = make_config()
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "strategy": "MULTI_QUERY",
                "reasoning": "Complex multi-part question",
                "searches": [
                    {"query": "RAG architecture", "k": 5},
                    {"query": "vector database performance", "k": 5},
                ],
                "expected_rounds": 2,
            }
        )
    )

    planner = QueryPlanner(llm, "default")
    plan = await planner.plan("Compare RAG with vector DB performance")

    assert plan.strategy == SearchStrategy.MULTI_QUERY
    assert len(plan.steps) == 2
    assert plan.expected_rounds == 2


@pytest.mark.asyncio
async def test_plan_filtered_strategy():
    """Test that a filtered question gets FILTERED strategy."""
    config = make_config()
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "strategy": "FILTERED",
                "reasoning": "User wants results from a specific date range",
                "searches": [
                    {
                        "query": "recent papers on RAG",
                        "filter": {"op": "gt", "field": "timestamp", "value": "1700000000"},
                        "k": 10,
                    }
                ],
                "expected_rounds": 1,
            }
        )
    )

    planner = QueryPlanner(llm, "default")
    plan = await planner.plan("Papers about RAG from 2024")

    assert plan.strategy == SearchStrategy.FILTERED
    assert len(plan.steps) == 1
    assert plan.steps[0].filter is not None


@pytest.mark.asyncio
async def test_plan_fallback_on_invalid_json():
    """Test that invalid JSON falls back to DIRECT strategy."""
    config = make_config()
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response("This is not JSON at all")

    planner = QueryPlanner(llm, "default")
    plan = await planner.plan("What is HNSW?")

    assert plan.strategy == SearchStrategy.DIRECT
    assert len(plan.steps) == 1
    assert plan.steps[0].query == "What is HNSW?"


@pytest.mark.asyncio
async def test_plan_from_tool_call():
    """Test that tool calls are properly parsed into plans."""
    config = make_config()
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        "",
        tool_calls=[
            {
                "id": "call_1",
                "name": "vector_search",
                "arguments": {"query": "HNSW algorithm", "k": 10, "collection": "papers"},
            }
        ],
    )

    planner = QueryPlanner(llm, "default")
    plan = await planner.plan("Explain HNSW")

    assert plan.strategy == SearchStrategy.DIRECT
    assert plan.steps[0].query == "HNSW algorithm"
    assert plan.steps[0].collection == "papers"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
