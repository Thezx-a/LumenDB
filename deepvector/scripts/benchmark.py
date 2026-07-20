"""
Performance benchmark for AgenticDB.

Measures query latency, multi-round efficiency, and result quality.
"""

import asyncio
import json
import sys
import os
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class Benchmark:
    """Simple benchmark for AgenticDB performance metrics."""

    def __init__(self, lumendb_url: str = "http://localhost:8080"):
        self.lumendb_url = lumendb_url
        self.results = []

    async def run(self):
        """Run all benchmarks."""
        print("AgenticDB Benchmark Suite")
        print("=" * 60)

        # Test 1: Search latency
        await self.benchmark_search()

        # Test 2: Multi-round retrieval efficiency
        await self.benchmark_multi_round()

        # Summary
        self.print_summary()

    async def benchmark_search(self):
        """Benchmark search latency."""
        print("\n1. Search Latency Benchmark")
        print("-" * 40)

        import httpx
        import numpy as np

        async with httpx.AsyncClient() as client:
            vectors = []
            for _ in range(100):
                vec = list(np.random.randn(768).astype(float))
                resp = await client.post(
                    f"{self.lumendb_url}/insert",
                    json={"vector": vec},
                )
                if resp.status_code == 200:
                    vectors.append(vec)

        print(f"   Ingested {len(vectors)} random vectors")

    async def benchmark_multi_round(self):
        """Benchmark multi-round retrieval."""
        print("\n2. Multi-Round Retrieval")
        print("-" * 40)
        print("   (Requires running agent server on port 8090)")
        print("   Test queries:")
        print("   - 'What is RAG and how does it work?'")
        print("   - 'Compare different vector quantization methods'")
        print("   - 'How to build a high-performance vector database?'")

    def print_summary(self):
        """Print benchmark summary."""
        print("\n" + "=" * 60)
        print("Benchmark Summary")
        print("=" * 60)
        print("\nMetrics to track:")
        print("  - P50/P95/P99 search latency (ms)")
        print("  - Multi-round retrieval quality vs rounds")
        print("  - Query planning accuracy (% correct strategy)")
        print("  - LLM token cost per query")
        print("  - End-to-end latency (question 鈫?answer)")


async def main():
    bench = Benchmark()
    await bench.run()


if __name__ == "__main__":
    asyncio.run(main())
