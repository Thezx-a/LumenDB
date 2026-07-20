"""
Demo data for AgenticDB — sample documents for testing.
"""

SAMPLE_DOCUMENTS = [
    {
        "text": "RAG (Retrieval-Augmented Generation) 是一种将检索系统与大语言模型结合的技术架构。通过从外部知识库检索相关文档，LLM能够生成更准确、更有依据的回答。RAG的核心流程包括：查询理解、文档检索、上下文组装和答案生成。",
        "tags": "topic:RAG,type:overview,lang:zh",
    },
    {
        "text": "HNSW (Hierarchical Navigable Small World) 是一种高效的近似最近邻搜索算法。它通过构建多层图结构实现快速搜索：顶层包含少量节点用于粗粒度导航，底层包含所有节点用于精确搜索。HNSW的搜索时间复杂度为O(log n)。",
        "tags": "topic:HNSW,type:theory,lang:zh",
    },
    {
        "text": "Product Quantization (PQ) 是一种向量压缩技术，通过将高维向量分割成多个子空间并对每个子空间进行聚类量化，实现大幅压缩。例如，一个768维的float32向量可以通过PQ压缩到64字节，压缩比达到48倍。",
        "tags": "topic:quantization,type:theory,lang:zh",
    },
    {
        "text": "mmap (memory-mapped file) 是一种将文件映射到进程地址空间的技术。使用mmap可以实现零拷贝读取，因为操作系统会按需将文件页面加载到内存中。这对于向量数据库的存储引擎非常有用，可以避免额外的数据复制。",
        "tags": "topic:storage,type:theory,lang:zh",
    },
    {
        "text": "向量数据库的核心性能指标包括：召回率(Recall)、查询延迟(Latency)和吞吐量(Throughput)。在百万级数据集上，优秀的向量数据库应该达到95%以上的召回率，同时保持毫秒级的查询延迟。",
        "tags": "topic:performance,type:benchmark,lang:zh",
    },
    {
        "text": "C++20 引入了协程(coroutine)支持，使得异步编程更加简洁。co_await、co_return和co_yield是三个核心关键字。协程可以实现零开销的异步操作，避免了回调地狱和线程开销。",
        "tags": "topic:cpp,type:theory,lang:zh",
    },
    {
        "text": "LSM-Tree (Log-Structured Merge-Tree) 是一种写优化的存储引擎。它将写操作首先记录到WAL和MemTable中，然后定期合并到SSTable文件中。LSM-Tree的写入吞吐量很高，但读取时需要检查多个层级。",
        "tags": "topic:storage,type:theory,lang:zh",
    },
    {
        "text": "元数据过滤是向量搜索的重要功能。用户可以通过标签、时间戳、类别等维度对搜索结果进行过滤。例如，只搜索特定类别下的文档，或者只返回最近添加的结果。过滤可以在搜索前(pre-filter)或搜索后(post-filter)执行。",
        "tags": "topic:filtering,type:overview,lang:zh",
    },
    {
        "text": "Pybind11 是一个轻量级的C++/Python绑定库，可以轻松地将C++函数和类暴露给Python。它使用纯头文件实现，支持现代C++特性，生成的Python模块性能接近原生C++代码。",
        "tags": "topic:python,type:theory,lang:zh",
    },
    {
        "text": "MCP (Model Context Protocol) 是一种标准化的协议，用于连接AI模型与外部工具和数据源。通过MCP，任何AI代理框架都可以以统一的方式调用外部服务，实现工具调用和数据访问。",
        "tags": "topic:MCP,type:overview,lang:zh",
    },
    {
        "text": "向量搜索的应用场景包括：语义搜索、推荐系统、图像检索、代码搜索、异常检测和问答系统。在生产环境中，通常需要结合元数据过滤、分层索引和缓存等技术来满足不同的性能需求。",
        "tags": "topic:applications,type:overview,lang:zh",
    },
    {
        "text": "Agent (智能体) 是一种能够自主规划、执行任务并根据反馈调整策略的AI系统。Agentic AI的核心能力包括：任务分解、工具使用、记忆管理和自我反思。ReAct (Reasoning + Acting) 是最经典的Agent范式之一。",
        "tags": "topic:agent,type:overview,lang:zh",
    },
    {
        "text": "Inner Product (内积) 和 Cosine Similarity (余弦相似度) 是两种常用的向量相似度度量。内积计算两个向量的点积，而余弦相似度计算两个向量夹角的余弦值。当向量已归一化时，两者等价。",
        "tags": "topic:distance,type:theory,lang:zh",
    },
    {
        "text": "AVX2 (Advanced Vector Extensions 2) 是Intel/AMD处理器的SIMD指令集，可以同时处理8个32位浮点数。利用AVX2可以将向量距离计算的性能提升4-8倍。向量数据库的性能优化通常从SIMD指令集开始。",
        "tags": "topic:SIMD,type:optimization,lang:zh",
    },
    {
        "text": "多轮检索(Multi-round Retrieval)是一种迭代式的搜索策略。第一轮进行初步搜索，然后评估结果质量。如果结果不够好，系统会自动生成改进的查询进行第二轮搜索，直到结果满足质量要求。",
        "tags": "topic:retrieval,type:advanced,lang:zh",
    },
]


async def insert_demo_data(deepvector_url: str = "http://localhost:8080"):
    """Insert sample documents into DeepVector."""
    import httpx

    print(f"Inserting {len(SAMPLE_DOCUMENTS)} documents into DeepVector...")

    async with httpx.AsyncClient() as client:
        for i, doc in enumerate(SAMPLE_DOCUMENTS):
            try:
                # First embed the text
                embed_resp = await client.post(
                    f"{deepvector_url}/embed",
                    json={"text": doc["text"]},
                )
                if embed_resp.status_code == 200:
                    vector = embed_resp.json()["vector"]
                else:
                    print(f"  Embed failed for doc {i}, using random vector")
                    import random
                    vector = [random.random() for _ in range(768)]

                # Then insert
                insert_resp = await client.post(
                    f"{deepvector_url}/insert",
                    json={"vector": vector},
                )
                if insert_resp.status_code == 200:
                    doc_id = insert_resp.json()["ids"][0]
                    print(f"  [{i+1}/{len(SAMPLE_DOCUMENTS)}] Inserted doc {doc_id}")
                else:
                    print(f"  [{i+1}/{len(SAMPLE_DOCUMENTS)}] Insert failed: {insert_resp.text}")

            except Exception as e:
                print(f"  [{i+1}/{len(SAMPLE_DOCUMENTS)}] Error: {e}")

    print("Done!")


if __name__ == "__main__":
    import asyncio
    asyncio.run(insert_demo_data())
