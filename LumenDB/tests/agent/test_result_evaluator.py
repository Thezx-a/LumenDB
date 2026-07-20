"""Tests for the ResultEvaluator."""

import asyncio
import json
import pytest
from unittest.mock import AsyncMock

from agent.llm.router import LLMRouter, LLMResponse
from agent.engine.result_evaluator import ResultEvaluator


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
async def test_evaluate_good_results():
    """Test evaluation of good quality results."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "score": 0.85,
                "relevance": 0.9,
                "coverage": 0.8,
                "sufficiency": 0.85,
                "feedback": "Results are highly relevant and cover the main aspects",
                "should_continue": False,
            }
        )
    )

    evaluator = ResultEvaluator(llm, quality_threshold=0.7)
    results = [
        {"id": 1, "distance": 0.2, "text": "RAG is Retrieval-Augmented Generation"},
        {"id": 2, "distance": 0.3, "text": "RAG combines retrieval with generation"},
    ]

    result = await evaluator.evaluate("What is RAG?", results)

    assert result.score == 0.85
    assert result.relevance == 0.9
    assert result.should_continue is False


@pytest.mark.asyncio
async def test_evaluate_poor_results():
    """Test evaluation of poor quality results triggers continuation."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "score": 0.3,
                "relevance": 0.2,
                "coverage": 0.3,
                "sufficiency": 0.3,
                "feedback": "Results are mostly irrelevant",
                "should_continue": True,
            }
        )
    )

    evaluator = ResultEvaluator(llm, quality_threshold=0.7)
    results = [{"id": 1, "distance": 0.9, "text": "Unrelated content"}]

    result = await evaluator.evaluate("What is HNSW?", results)

    assert result.score == 0.3
    assert result.should_continue is True


@pytest.mark.asyncio
async def test_evaluate_empty_results():
    """Test evaluation with no results."""
    llm = AsyncMock(spec=LLMRouter)

    evaluator = ResultEvaluator(llm, quality_threshold=0.7)
    result = await evaluator.evaluate("What is?", [])

    assert result.score == 0.0
    assert result.should_continue is True
    assert "No results" in result.feedback


@pytest.mark.asyncio
async def test_evaluate_threshold_boundary():
    """Test that quality threshold properly gates continuation."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response(
        json.dumps(
            {
                "score": 0.69,
                "relevance": 0.7,
                "coverage": 0.6,
                "sufficiency": 0.7,
                "feedback": "Almost good enough",
                "should_continue": True,
            }
        )
    )

    evaluator = ResultEvaluator(llm, quality_threshold=0.7)
    results = [{"id": 1, "distance": 0.5, "text": "Some content"}]

    result = await evaluator.evaluate("Test?", results)

    # Score 0.69 < threshold 0.7, so should_continue should be True
    assert result.should_continue is True


@pytest.mark.asyncio
async def test_evaluate_handles_invalid_json():
    """Test graceful handling of invalid LLM response."""
    llm = AsyncMock(spec=LLMRouter)
    llm.chat.return_value = mock_llm_response("not json at all")

    evaluator = ResultEvaluator(llm, quality_threshold=0.7)
    results = [{"id": 1, "distance": 0.5, "text": "content"}]

    result = await evaluator.evaluate("Test?", results)

    # Should not crash, returns a default result
    assert result.score == 0.5
    assert result.should_continue is False  # parse failure = stop


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
