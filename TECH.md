# Technology Choices — Proven & Current

This document explains **why** each major dependency is used. The bar:

1. Still actively maintained (2024–2026)
2. Widely deployed in large enterprises / hyperscalers
3. Boring over trendy — prefer de-facto standards

## Core stack

| Layer | Choice | Why (enterprise signal) |
|-------|--------|-------------------------|
| Systems language | **C++17** (libs) + **C++20** (server) | Dominant for high-perf storage/search (RocksDB, Faiss-adjacent tooling, Redis modules). C++17 is the safe baseline; C++20 for coroutines in SkyNet. |
| Build | **CMake 3.20+** + Ninja | Industry default for C++ monorepos (Google, Meta, MSVC/CLion workflows). |
| Vector index | **HNSW** | Production ANN standard (Elasticsearch kNN, Milvus, Weaviate, Qdrant all offer HNSW). |
| Distance kernels | Scalar + optional **AVX2** | Same SIMD path used in Faiss / modern search engines on x86. |
| KV / LSM | MiniKV (WAL + MemTable + SST + Compaction) | Pattern from **LevelDB / RocksDB** — Meta, LinkedIn, etc. run RocksDB at scale. |
| Merging iterator | MemTable + SSTable heap merge | Direct LevelDB/RocksDB `MergingIterator` design. |
| JSON | **nlohmann/json** ~3.11 | De-facto C++ JSON library; stable API. |
| Python bindings | **pybind11** 2.13.x | Standard for production C++→Python (PyTorch historically, many scientific stacks). |
| Agent HTTP | **FastAPI** + **Pydantic v2** + **Uvicorn** | Dominant Python API stack (Microsoft, Netflix tutorials/prod patterns). Uses `lifespan` + `model_dump()` (current APIs). |
| HTTP client | **httpx** | Async-first; replaces ad-hoc urllib for agent↔DB calls. |
| Embeddings (local) | **sentence-transformers** 3.x + `all-MiniLM-L6-v2` | Hugging Face standard for local RAG demos; swap to OpenAI/Azure for prod. |
| Embeddings (cloud) | OpenAI / Azure-compatible `/v1/embeddings` | Enterprise default for managed embeddings. |
| LLM | **Ollama** (local) or **OpenAI API** | Ollama for offline; OpenAI/Azure for production SLAs. |
| Agent orchestration | Multi-round plan → search → evaluate | Same shape as production RAG agents (retrieve–grade–retry), without locking to a single agent framework. |
| MCP (optional) | Official **mcp** Python SDK | Anthropic Model Context Protocol — emerging but real; kept optional. |
| LangChain (optional) | **langchain-core** VectorStore | Use shared `langchain_core` packages (not deprecated `langchain.schema`). |
| Containers | **Docker** multi-stage + Compose | Universal packaging for Linux services on any host OS. |

## Explicit non-goals / avoided tech

| Avoided | Reason |
|---------|--------|
| Native Win32 port of epoll/select server | Enterprises run this class of DB on Linux containers; WSL2/Docker is the supported Windows path. |
| Pydantic v1 `.dict()` / FastAPI `on_event` | Deprecated; migrated to v2 `model_dump()` and `lifespan`. |
| Legacy `langchain.schema` / `langchain.vectorstores.base` | Replaced by `langchain_core`. |
| Unmaintained ANN libraries | HNSW is the mainstream choice; no exotic research-only indexes. |
| Homegrown embedding formats | Stick to float32 vectors + industry distances (L2 / IP / Cosine). |

## Windows policy

C++ networking and MiniKV I/O are **POSIX** (`socket`, `select`, `open`, `epoll` in MiniKV network). Supported runtimes:

1. **Linux / macOS** native  
2. **WSL2** on Windows  
3. **Docker Desktop** (`docker compose up`)

This matches how most companies ship Linux-first infra to Windows developer machines.

## Upgrade cadence

- Python deps: minor pins in `deepvector/requirements.txt` (upper caps on major versions).
- C++ FetchContent tags: pybind11 `v2.13.6`, GoogleTest `v1.15.2`, nlohmann/json `v3.11.3`.
- Re-evaluate annually; prefer LTS / widely tagged releases over bleeding-edge nightlies.
