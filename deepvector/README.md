<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++17"/>
  <img src="https://img.shields.io/badge/Build-CMake%20%2B%20Ninja-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake"/>
  <img src="https://img.shields.io/badge/Tests-28%20passing-brightgreen?style=for-the-badge" alt="Tests"/>
  <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge" alt="MIT"/>
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey?style=for-the-badge" alt="Platform"/>
</p>

<h1 align="center">⚡ DeepVector</h1>

<p align="center">
  <b>C++ Zero-Copy Vector Database for RAG</b><br/>
  <i>Embeddable · SIMD-Accelerated · Production-Ready</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Insert-30K%20vec%2Fs-orange" alt="Insert"/>
  <img src="https://img.shields.io/badge/Search_P50-150%C2%B5s-blue" alt="Search P50"/>
  <img src="https://img.shields.io/badge/Recall-99.5%25-brightgreen" alt="Recall"/>
  <img src="https://img.shields.io/badge/Compression-32%C3%97-purple" alt="Compression"/>
</p>

---

## What is DeepVector?

DeepVector is an **embedded** C++ vector database purpose-built for Retrieval-Augmented Generation (RAG). It lives inside your application as a static library — no separate server process, no network overhead, no latency penalty.

### Architecture

```mermaid
graph TB
    subgraph "Your Application"
        APP[Application Code]
    end

    subgraph "DeepVector"
        API[Collection API]
        HNSW[HNSW Graph Index<br/>SIMD AVX2]
        MKV[MiniKV Metadata<br/>LSM-Tree + Bloom Filter]
        VS[VectorStore<br/>mmap Zero-Copy]
        PQ[PQ 32x + SQ 4x<br/>Quantization]
        HTTP[HTTP Server<br/>REST + Auth]
        PY[Python Bindings<br/>pybind11]
    end

    APP --> API
    API --> HNSW
    API --> MKV
    API --> HTTP
    API --> PY
    HNSW --> VS
    VS --> PQ

    style HNSW fill:#e1f5fe
    style MKV fill:#f3e5f5
    style VS fill:#e8f5e9
    style PQ fill:#fff3e0
```

### How is it different?

| | DeepVector | Milvus / Qdrant | FAISS |
|---|---|---|---|
| **Architecture** | Embedded library | Client-Server | Index library only |
| **Metadata** | Full LSM-Tree store | Proprietary | None |
| **Zero-copy** | ✅ mmap throughout | ❌ serialization | ❌ copies |
| **Filtering** | SQL-like expressions | DSL | ❌ manual |
| **Quantization** | PQ + SQ built-in | ✅ | ✅ |
| **Language** | C++ core + Python | Go/Rust + Python | C++/Python |

---

## Quick Start

### C++ (3 lines to search)

```cpp
#include <deepvector/collection.h>
using namespace deepvector;

Collection coll({768, DistanceMetric::Cosine}, "./data");
coll.add(my_embedding);                              // returns id
auto results = coll.search(query_embedding, 10);     // top-10 results
```

### Docker (1 command)

```bash
docker compose up -d
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"vector":[0.1,0.2,...],"k":5}'
```

### Python (native bindings)

```python
import deepvector, numpy as np

cfg = deepvector.CollectionConfig()
cfg.dim = 768
coll = deepvector.Collection(cfg)
coll.add(np.random.randn(768).astype(np.float32))
results = coll.search(np.random.randn(768).astype(np.float32), 5)
```

---

## Search Flow

```mermaid
sequenceDiagram
    participant Client
    participant API as Collection API
    participant HNSW as HNSW Index
    participant VS as VectorStore
    participant MKV as MiniKV
    participant PQ as Quantizer

    Client->>API: search(query, k)
    API->>HNSW: ANN search (top-k)
    HNSW->>VS: mmap read vectors
    VS-->>HNSW: raw vectors
    HNSW-->>API: candidate IDs + distances

    API->>MKV: get metadata for IDs
    MKV-->>API: metadata map

    alt PQ enabled
        API->>PQ: decode quantized
        PQ-->>API: approximate vectors
    end

    API->>HNSW: re-rank with filter
    HNSW-->>API: filtered results
    API-->>Client: SearchResults
```

---

## Performance

**50K vectors · 128-dim · AVX2 · g++-12**

```mermaid
xychart-beta
    title "Search Latency (microseconds)"
    x-axis ["P50 Raw", "P50 PQ", "P99 Raw", "P99 PQ"]
    y-axis "Latency (us)" 0 --> 900
    bar [150, 80, 800, 400]
```

| Metric | Raw float32 | With PQ (32×) |
|--------|-------------|---------------|
| Insert throughput | ~30K vec/s | ~35K vec/s |
| Search latency P50 | ~150µs | ~80µs |
| Search latency P99 | ~800µs | ~400µs |
| Memory per vector | `dim × 4B` | `dim/32 × 1B` |
| Recall@10 | 99.5% | ~97% |

> Run it yourself: `cmake -B build -DENABLE_BENCHMARKS=ON && ./build/benchmarks/bench_hnsw`

---

## Memory Savings

```mermaid
pie title Memory per Vector (1024-dim)
    "Raw float32 (4KB)" : 4096
    "PQ compressed (32B)" : 32
    "Saved (4064B)" : 4064
```

| Vectors | Raw float32 | PQ (32×) | Savings |
|---------|-------------|----------|---------|
| 100K | 400 MB | 12.5 MB | 97% |
| 1M | 4 GB | 125 MB | 97% |
| 10M | 40 GB | 1.25 GB | 97% |

---

## Features

| Feature | Status | Description |
|---------|--------|-------------|
| HNSW graph index | ✅ | M=16, ef_construction=200 |
| SIMD distance (AVX2) | ✅ | L2, Inner Product, Cosine |
| Zero-copy mmap storage | ✅ | Instant restart via OS page cache |
| Metadata filtering | ✅ | Tree expressions (eq/gt/and/or) |
| PQ quantization | ✅ | k-means subspace, 32× compression |
| SQ int8 quantization | ✅ | Per-dimension scaling, 4× compression |
| HTTP REST API | ✅ | GET/POST/DELETE endpoints |
| Bearer auth | ✅ | Configurable API key |
| Python bindings | ✅ | pybind11 numpy zero-copy |
| Docker deployment | ✅ | Multi-stage, <100MB |
| gRPC / TLS | 🔜 | Planned |
| Prometheus metrics | 🔜 | Atomic counters exist |
| Distributed / sharding | 🔜 | Architecture reserved |

---

## Build

```bash
# Clone with submodules
git clone --recursive https://github.com/Thezx-a/DeepVector.git
cd DeepVector

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTS=ON -DCMAKE_CXX_COMPILER=g++-12
cmake --build build -j$(nproc)

# Test
ctest --test-dir build --output-on-failure

# Python bindings
cmake -B build -DENABLE_PYTHON=ON
cmake --build build
cd python && pip install -e .
```

---

## Documentation

| Document | What's Inside |
|----------|---------------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Design decisions, data flow, format specs |
| [INTERVIEW_QA.md](INTERVIEW_QA.md) | 78 deep-dive Q&A for interview prep |
| [TUTORIAL.md](TUTORIAL.md) | Step-by-step walkthrough |
| [API_REFERENCE.md](API_REFERENCE.md) | Full API documentation |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |

---

## Dependencies

All fetched automatically via CMake FetchContent:

```mermaid
graph LR
    DeepVector --> MiniKV[MiniKV<br/>LSM-Tree]
    DeepVector --> SkyNet[SkyNet<br/>C++20 Coroutines]
    DeepVector --> pybind11[pybind11<br/>Python Bindings]
    DeepVector --> json[nlohmann/json<br/>JSON Parser]
    DeepVector --> gtest[GoogleTest<br/>Unit Testing]

    style DeepVector fill:#e1f5fe
    style MiniKV fill:#f3e5f5
    style SkyNet fill:#e8f5e9
```

| Library | Purpose |
|---------|---------|
| [MiniKV](https://github.com/Thezx-a/MiniKV) | LSM-Tree metadata storage |
| [SkyNet](https://github.com/Thezx-a/SkyNet) | C++20 coroutine network (server) |
| pybind11 | Python bindings |
| nlohmann/json | HTTP JSON parsing |
| GoogleTest | Unit testing |

---

## License

[MIT](LICENSE) — free to use, modify, and distribute.
