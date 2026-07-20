"""
AgenticDB Demo — showcases agent-powered natural language search.
"""

import asyncio
import sys
import os

# Add parent dir to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from agent.config import AgenticDBConfig
from agent.llm.router import LLMRouter
from agent.engine.multi_round import MultiRoundEngine


async def demo_simple_query():
    """Demo 1: Simple direct query."""
    print("\n" + "=" * 60)
    print("Demo 1: Simple Direct Query")
    print("=" * 60)

    config = AgenticDBConfig()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    question = "What is RAG?"
    print(f"\nQuestion: {question}")
    print("-" * 40)

    result = await engine.retrieve(question)

    print(f"\nStrategy: {result.plan.strategy.value}")
    print(f"Rounds: {result.rounds}")
    print(f"Quality: {result.quality_score:.2f}")
    print(f"Queries tried: {result.all_queries_tried}")
    print(f"\nAnswer:\n{result.answer}")
    print(f"\nDocuments found: {len(result.documents)}")

    await engine.close()


async def demo_filtered_query():
    """Demo 2: Filtered search."""
    print("\n" + "=" * 60)
    print("Demo 2: Filtered Search")
    print("=" * 60)

    config = AgenticDBConfig()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    question = "Find articles about quantization from recent sources"
    print(f"\nQuestion: {question}")
    print("-" * 40)

    result = await engine.retrieve(question)

    print(f"\nStrategy: {result.plan.strategy.value}")
    print(f"Rounds: {result.rounds}")
    print(f"Quality: {result.quality_score:.2f}")
    print(f"\nAnswer:\n{result.answer}")

    await engine.close()


async def demo_complex_query():
    """Demo 3: Complex multi-aspect query."""
    print("\n" + "=" * 60)
    print("Demo 3: Complex Multi-Aspect Query")
    print("=" * 60)

    config = AgenticDBConfig()
    llm = LLMRouter(config.llm)
    engine = MultiRoundEngine(config, llm)

    question = "Compare HNSW and IVF indexing algorithms for vector search performance"
    print(f"\nQuestion: {question}")
    print("-" * 40)

    result = await engine.retrieve(question)

    print(f"\nStrategy: {result.plan.strategy.value}")
    print(f"Rounds: {result.rounds}")
    print(f"Quality: {result.quality_score:.2f}")
    print(f"Queries tried: {result.all_queries_tried}")
    print(f"\nAnswer:\n{result.answer}")

    await engine.close()


async def main():
    """Run all demos."""
    print("AgenticDB Demo")
    print("Agent-Native Vector Database")

    try:
        await demo_simple_query()
        await demo_filtered_query()
        await demo_complex_query()
    except Exception as e:
        print(f"\nDemo error: {e}")
        print("Make sure LumenDB server is running on localhost:8080")
        print("And Ollama is running with qwen2.5 model loaded")


if __name__ == "__main__":
    asyncio.run(main())
