# LumenDB Tutorial: From Zero to RAG

A complete, practical guide to building and running a RAG pipeline with LumenDB.

## Prerequisites

- **Linux** (Ubuntu 22.04 recommended) or WSL2 on Windows
- **g++-12** (`sudo apt-get install g++-12`)
- **cmake** 3.16+ and **ninja** (`sudo apt-get install cmake ninja-build`)
- **Python 3.10+** with pip
- **4GB RAM** minimum (8GB recommended for 1M+ vectors)

---

## Part 1: Building LumenDB (5 min)

### 1.1 Clone and enter

```bash
git clone --recurse-submodules https://github.com/Thezx-a/LumenDB.git
cd LumenDB
```

### 1.2 Configure with CMake

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTS=ON \
  -DCMAKE_CXX_COMPILER=g++-12
```

Expected output:
```
-- The CXX compiler identification is GNU 12.3.0
-- Detecting CXX compiler ABI info - done
-- Adding MiniKV subdirectory...
-- Adding SkyNet subdirectory...
-- Fetching nlohmann/json v3.11.3...
-- Configuring done
-- Generating done
```

### 1.3 Build

```bash
cmake --build build -j$(nproc)
```

Expected output:
```
[12/18] Building CXX object vendor/MiniKV/.../db_impl.cpp.o
[15/18] Building CXX object src/index/hnsw.cpp.o
[16/18] Building CXX object src/collection.cpp.o
[17/18] Linking CXX static library lib/liblumendb.a
[18/18] Linking CXX executable build/lumendb_server
```

### 1.4 Run tests

```bash
ctest --test-dir build --output-on-failure
```

Expected output:
```
Test project .../lumenDB/build
    Start 1: DistanceTest.L2DistanceZero
    ...
    Start 28: CollectionFilterTest.GetMeta
100% tests passed, 0 tests failed out of 28
```

### 1.5 Build Python bindings (optional)

```bash
cmake -B build_py -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_PYTHON=ON \
  -DCMAKE_CXX_COMPILER=g++-12

cmake --build build_py -j$(nproc)
cd python && pip install -e .
```

Verify:
```bash
python -c "import lumendb; print(dir(lumendb))"
```

---

## Part 2: Your First Collection (5 min)

Create `tutorial_part2.cpp`:

```cpp
#include <lumendb/collection.h>
#include <iostream>
#include <random>
#include <cmath>

using namespace lumendb;

int main() {
    // --- Configure ---
    CollectionConfig cfg;
    cfg.dim = 128;
    cfg.metric = DistanceMetric::L2;
    cfg.hnsw_m = 16;
    cfg.hnsw_ef_construction = 200;
    cfg.hnsw_ef_search = 50;

    // --- Create ---
    Collection coll(cfg, "/tmp/lumendb_tutorial");
    std::cout << "Created collection, dim=" << coll.dim() << std::endl;

    // --- Generate random vectors and insert ---
    std::mt19937 rng(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    const int N = 1000;
    std::vector<float> vectors(N * cfg.dim);
    for (int i = 0; i < N; ++i) {
        for (size_t d = 0; d < cfg.dim; ++d) {
            vectors[i * cfg.dim + d] = dist(rng);
        }
        coll.add(&vectors[i * cfg.dim]);
    }
    std::cout << "Inserted " << coll.size() << " vectors" << std::endl;

    // --- Search ---
    std::vector<float> query(cfg.dim, 0.0f);
    auto results = coll.search(query.data(), 5);

    std::cout << "Top 5 results for origin query:" << std::endl;
    for (auto& r : results) {
        const float* v = coll.getVector(r.id);
        // Compute actual L2 norm of the returned vector
        float norm = 0.0f;
        for (size_t d = 0; d < cfg.dim; ++d) norm += v[d] * v[d];
        std::cout << "  id=" << r.id
                  << "  distance=" << r.distance
                  << "  vector_norm=" << std::sqrt(norm)
                  << std::endl;
    }

    // --- Self-search: search with an inserted vector as query ---
    std::cout << "\nSelf-search (one of the inserted vectors):" << std::endl;
    auto self_results = coll.search(&vectors[0], 5);
    for (auto& r : self_results) {
        std::cout << "  id=" << r.id << "  distance=" << r.distance;
        if (r.id == 1) std::cout << "  <- self (should be ~0)";
        std::cout << std::endl;
    }

    return 0;
}
```

Compile and run:

```bash
g++-12 -std=c++17 -O2 -I include \
  tutorial_part2.cpp \
  build/liblumendb.a \
  build/vendor/MiniKV/libminikv.a \
  -lpthread -o tutorial_part2

./tutorial_part2
```

Expected output:
```
Created collection, dim=128
Inserted 1000 vectors
Top 5 results for origin query:
  id=42   distance=5.21832   vector_norm=5.21832
  id=517  distance=6.48321   vector_norm=6.48321
  ...
Self-search (one of the inserted vectors):
  id=1  distance=0  <- self (should be ~0)
  ...
```

---

## Part 3: Metadata + Filtering (5 min)

Create `tutorial_part3.cpp`:

```cpp
#include <lumendb/collection.h>
#include <lumendb/filter.h>
#include <lumendb/storage/document_store.h>
#include <iostream>
#include <random>

using namespace lumendb;
using namespace lumendb::storage;

int main() {
    CollectionConfig cfg;
    cfg.dim = 64;
    cfg.metric = DistanceMetric::Cosine;
    cfg.hnsw_m = 16;
    cfg.hnsw_ef_construction = 100;
    cfg.hnsw_ef_search = 40;

    Collection coll(cfg, "/tmp/lumendb_tutorial_meta");

    // Insert with metadata
    std::mt19937 rng(123);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    for (int i = 0; i < 100; ++i) {
        std::vector<float> v(cfg.dim);
        for (size_t d = 0; d < cfg.dim; ++d) v[d] = dist(rng);

        DocumentMeta meta;
        meta.text = "document_" + std::to_string(i);
        meta.tags = (i % 3 == 0) ? "science" : (i % 3 == 1) ? "tech" : "art";
        meta.timestamp = 1000000 + i * 1000;

        coll.add(v.data(), meta);
    }
    std::cout << "Inserted " << coll.size() << " vectors with metadata" << std::endl;

    // --- Search with tag filter ---
    std::vector<float> query(cfg.dim, 0.0f);
    auto tagFilter = FilterNode::eq("tags", "science");

    auto results = coll.searchWithFilter(query.data(), 5, tagFilter);
    std::cout << "\nSearch with filter tags=science:" << std::endl;
    for (auto& r : results) {
        auto meta = coll.getMeta(r.id);
        std::cout << "  id=" << r.id
                  << "  text=" << meta->text
                  << "  tags=" << meta->tags
                  << "  dist=" << r.distance
                  << std::endl;
    }
    std::cout << "  (all should have tags=science)" << std::endl;

    // --- Search with timestamp filter ---
    auto tsFilter = FilterNode::gt("timestamp", "1050000");
    auto tsResults = coll.searchWithFilter(query.data(), 5, tsFilter);
    std::cout << "\nSearch with filter timestamp>1050000:" << std::endl;
    for (auto& r : tsResults) {
        auto meta = coll.getMeta(r.id);
        std::cout << "  id=" << r.id
                  << "  timestamp=" << meta->timestamp
                  << "  dist=" << r.distance
                  << std::endl;
    }

    // --- Compound filter (AND) ---
    auto compound = FilterNode::andAlso(
        FilterNode::contains("tags", "tech"),
        FilterNode::gt("timestamp", "1030000")
    );
    auto compResults = coll.searchWithFilter(query.data(), 5, compound);
    std::cout << "\nSearch with tag contains 'tech' AND timestamp>1030000:" << std::endl;
    for (auto& r : compResults) {
        auto meta = coll.getMeta(r.id);
        std::cout << "  id=" << r.id
                  << "  tags=" << meta->tags
                  << "  ts=" << meta->timestamp
                  << std::endl;
    }

    return 0;
}
```

Compile and run:

```bash
g++-12 -std=c++17 -O2 -I include \
  tutorial_part3.cpp \
  build/liblumendb.a \
  build/vendor/MiniKV/libminikv.a \
  -lpthread -o tutorial_part3

./tutorial_part3
```

---

## Part 4: Python Bindings (5 min)

Ensure you built Python bindings in Part 1.5. Then create `tutorial_part4.py`:

```python
import numpy as np
import lumendb

# --- Configure ---
cfg = lumendb.CollectionConfig()
cfg.dim = 128
cfg.metric = lumendb.DistanceMetric.L2
cfg.hnsw_m = 16
cfg.hnsw_ef_construction = 200
cfg.hnsw_ef_search = 50

# --- Create ---
coll = lumendb.Collection(cfg, "/tmp/lumendb_tutorial_py")

# --- Insert 1000 random vectors ---
rng = np.random.default_rng(42)
vectors = rng.normal(0, 1, (1000, cfg.dim)).astype(np.float32)
for vec in vectors:
    coll.add(vec)

print(f"Inserted {len(coll)} vectors, dim={coll.dim}")

# --- Search ---
query = np.zeros(cfg.dim, dtype=np.float32)
results = coll.search(query, k=5)

print("Top 5 results:")
for r in results:
    print(f"  id={r.id}  distance={r.distance:.4f}")

# --- Insert with metadata ---
meta = lumendb.DocumentMeta()
meta.text = "A document about vector databases"
meta.tags = "databases,vectors,search"
meta.timestamp = 1700000000

doc_vec = rng.normal(0, 1, cfg.dim).astype(np.float32)
doc_id = coll.add_with_meta(doc_vec, meta)
print(f"\nInserted document with id={doc_id}")

retrieved = coll.get_meta(doc_id)
if retrieved:
    print(f"Retrieved: text='{retrieved.text}', tags='{retrieved.tags}'")

# --- Zero-copy vector access ---
vec_view = coll.get_vector(doc_id)
if vec_view is not None:
    print(f"Vector shape: {vec_view.shape}, dtype: {vec_view.dtype}")
    print(f"First 5 values: {vec_view[:5]}")

# --- Filtered search ---
filter_expr = lumendb.FilterNode.contains("tags", "databases")
filtered = coll.search_with_filter(query, k=5, filter=filter_expr)
print(f"\nFiltered search results: {len(filtered)}")
for r in filtered:
    m = coll.get_meta(r.id)
    if m:
        print(f"  id={r.id}  tags='{m.tags}'  dist={r.distance:.4f}")
```

Run:

```bash
python tutorial_part4.py
```

---

## Part 5: Running the HTTP Server (5 min)

### 5.1 Start the server

```bash
./build/lumendb_server --host 0.0.0.0 --port 8080 --dim 128 --data-dir /tmp/lumendb_server_data
```

Expected output:
```
LumenDB initialized, dimension=128
LumenDB server listening on 0.0.0.0:8080
Press Ctrl+C to stop
```

### 5.2 Insert vectors via curl

```bash
# Insert a single vector
curl -s -X POST http://localhost:8080/insert \
  -H "Content-Type: application/json" \
  -d '{"vector": [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8]}'

# Insert batch
curl -s -X POST http://localhost:8080/insert \
  -H "Content-Type: application/json" \
  -d '{"vectors": [[1.0,0.0], [0.0,1.0], [0.5,0.5]]}'
```

### 5.3 Search

```bash
curl -s -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"vector": [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8], "k": 3}' | python -m json.tool
```

Expected output:
```json
{
    "results": [
        {"id": 1, "distance": 0.0},
        {"id": 2, "distance": 1.414},
        {"id": 3, "distance": 2.236}
    ]
}
```

### 5.4 Health and stats

```bash
curl -s http://localhost:8080/health | python -m json.tool
curl -s http://localhost:8080/stats | python -m json.tool
```

### 5.5 With authentication

```bash
# Start server with API key
./build/lumendb_server --api-key "my-secret-key" --port 8080 --dim 128

# Request must include Authorization header
curl -s -H "Authorization: Bearer my-secret-key" \
  http://localhost:8080/health

# Without key → 401
curl -s http://localhost:8080/health
```

### 5.6 Docker deployment

```bash
docker build -t lumendb .
docker run -p 8080:8080 -v $(pwd)/data:/data lumendb
```

Or with docker-compose:

```bash
docker-compose up -d
```

---

## Part 6: Full RAG Pipeline (10 min)

A complete Retrieval-Augmented Generation pipeline using Python.

### 6.1 Install dependencies

```bash
pip install lumendb numpy openai sentence-transformers langchain
```

### 6.2 Create `tutorial_rag.py`

```python
"""
Full RAG Pipeline: Load documents → Chunk → Embed → Store → Query → Generate
"""
import numpy as np
import lumendb

# ============================================================
# Step 1: Setup LumenDB
# ============================================================
cfg = lumendb.CollectionConfig()
cfg.dim = 384  # all-MiniLM-L6-v2 output dimension
cfg.metric = lumendb.DistanceMetric.Cosine
cfg.hnsw_m = 32
cfg.hnsw_ef_construction = 200
cfg.hnsw_ef_search = 100

coll = lumendb.Collection(cfg, "/tmp/lumendb_rag")

# ============================================================
# Step 2: Load embedding model (offline, no API key needed)
# ============================================================
from sentence_transformers import SentenceTransformer
embedder = SentenceTransformer("all-MiniLM-L6-v2")

# ============================================================
# Step 3: Load and chunk documents
# ============================================================
documents = [
    "LumenDB is a C++ vector database optimized for RAG workloads. "
    "It uses HNSW for approximate nearest neighbor search with SIMD acceleration.",

    "Product Quantization (PQ) compresses vectors by splitting them into subspaces "
    "and quantizing each subspace independently. LumenDB supports PQ with configurable "
    "M (number of subspaces) and K=256 centroids per subspace.",

    "The HNSW algorithm builds a multi-layer navigable small-world graph. "
    "Each node is assigned a random level from an exponential distribution. "
    "Search traverses from the top layer down, using beam search at layer 0.",

    "LumenDB stores vectors using memory-mapped files (mmap) for zero-copy access. "
    "This enables sub-100ms startup time and leverages the OS page cache for "
    "automatic memory management.",

    "MiniKV is an embedded LSM-tree key-value store used for metadata storage. "
    "It provides write-ahead logging for durability and Bloom filters for fast "
    "key lookups during filtered searches.",

    "RAG (Retrieval-Augmented Generation) combines vector search with LLM generation. "
    "Documents are embedded and stored in a vector database. At query time, relevant "
    "documents are retrieved and passed to the LLM as context for grounded answers.",

    "Scalar Quantization (SQ) maps each float dimension to an int8 value using "
    "per-dimension min/max scaling. This achieves 4x compression with minimal "
    "accuracy loss compared to raw float32 vectors.",

    "LumenDB supports metadata filtering through an AST-based filter system. "
    "Filters can combine equality, contains, greater-than, less-than operations "
    "using AND, OR, and NOT logical operators. Filtered search progressively "
    "widens ef_search to find enough matching candidates.",
]

print(f"Loaded {len(documents)} documents")

# ============================================================
# Step 4: Embed and store
# ============================================================
import time

embeddings = embedder.encode(documents, convert_to_numpy=True).astype(np.float32)
print(f"Generated {len(embeddings)} embeddings, shape={embeddings.shape}")

for i, (doc, emb) in enumerate(zip(documents, embeddings)):
    meta = lumendb.DocumentMeta()
    meta.text = doc
    meta.tags = f"doc_{i}"
    meta.timestamp = int(time.time())
    coll.add_with_meta(emb, meta)

print(f"Stored {len(coll)} vectors in LumenDB")

# ============================================================
# Step 5: Query the RAG pipeline
# ============================================================
queries = [
    "How does LumenDB store vectors on disk?",
    "What is Product Quantization and how does it work?",
    "Explain the HNSW algorithm for vector search.",
]

for query_text in queries:
    print(f"\n{'='*60}")
    print(f"Query: {query_text}")

    # Embed the query
    query_emb = embedder.encode(query_text, convert_to_numpy=True).astype(np.float32)

    # Search LumenDB
    results = coll.search(query_emb, k=3)

    print(f"  Retrieved {len(results)} documents:")
    for j, r in enumerate(results):
        meta = coll.get_meta(r.id)
        if meta:
            # Show first 120 chars of each document
            snippet = meta.text[:120] + "..." if len(meta.text) > 120 else meta.text
            print(f"  [{j}] dist={r.distance:.4f} | {snippet}")

    # In a real RAG system, you would now pass these documents to an LLM:
    # context = "\n".join([coll.get_meta(r.id).text for r in results])
    # prompt = f"Context:\n{context}\n\nQuestion: {query_text}\nAnswer:"
    # response = openai.ChatCompletion.create(model="gpt-4", messages=[{"role": "user", "content": prompt}])

print("\nRAG pipeline complete!")
```

Run:

```bash
python tutorial_rag.py
```

### 6.3 With OpenAI (requires API key)

```python
# Add this after the LumenDB search in the loop above:
import openai
openai.api_key = "sk-..."

context = "\n\n".join([coll.get_meta(r.id).text for r in results])
response = openai.ChatCompletion.create(
    model="gpt-4",
    messages=[{
        "role": "system",
        "content": "Answer the question using only the provided context. If the context doesn't contain the answer, say so."
    }, {
        "role": "user",
        "content": f"Context:\n{context}\n\nQuestion: {query_text}"
    }]
)
print(f"  Generated answer: {response.choices[0].message.content}")
```

---

## Part 7: Performance Tuning (5 min)

### 7.1 Choosing `M` (max neighbors)

| M | Memory/Node | Recall@10 | Insert Speed | Best For |
|---|-------------|-----------|-------------|----------|
| 8 | ~140 bytes | 93-95% | Fastest | Low memory, high throughput |
| 16 | ~270 bytes | 97-99% | Fast | **Default, general purpose** |
| 32 | ~530 bytes | 99-99.5% | Moderate | High recall requirements |
| 64 | ~1KB | 99.5%+ | Slower | Maximum recall, small datasets |

Rule of thumb: `M = 4 × sqrt(dim)` for balanced performance. For 768-dim: `M = 4 × sqrt(768) ≈ 110` — but in practice, M=16-32 is sufficient because HNSW recall plateaus beyond a point.

### 7.2 Choosing `ef_construction`

| ef_construction | Build Time | Recall@10 | Note |
|-----------------|------------|-----------|------|
| 100 | 1× (baseline) | 95% | Fastest build |
| 200 | 1.5× | 97-98% | **Default, good balance** |
| 400 | 2.5× | 99% | High quality builds |
| 800 | 5× | 99.5% | Maximum quality, 1M+ datasets |

Higher `ef_construction` improves graph quality at build time. Search-time `ef_search` can compensate for lower `ef_construction` in many cases.

### 7.3 When to use PQ vs raw

| Scenario | Recommendation | Rationale |
|----------|---------------|-----------|
| <10K vectors | Raw (no PQ) | Memory is small enough |
| 10K-100K vectors | SQ (4× compression) | Fast, low quality loss |
| 100K-1M vectors | PQ M=dim/4, K=256 | 32× compression, 95%+ recall |
| 1M-10M vectors | PQ M=dim/8, K=256 | 64× compression, ~90% recall |
| >10M vectors | PQ + IVF or sharding | Beyond single-node capacity |

Configure PQ:
```cpp
CollectionConfig cfg;
cfg.dim = 768;
cfg.use_pq = true;
cfg.pq_M = 96;   // 768/8 = 96 subspaces, 8-dim each
cfg.pq_K = 256;  // 1-byte codes
```

PQ training is automatic: after `kPQTrainThreshold` (256) vectors are inserted, training triggers. Training data accumulates in `Collection::train_data_` until the threshold is met.

### 7.4 Memory planning

```
Total memory = vector_data + index_overhead + metadata

vector_data (raw)  = N × dim × 4 bytes
vector_data (PQ)   = N × M bytes  (M = dim/4 → 32× compression)
vector_data (SQ)   = N × dim bytes (int8, 4× compression)

index_overhead      = N × (sizeof(HNSWNode) + avg_neighbors × 8)
                    ≈ N × (40 + M_max0_ × 8)  for M=16
                    ≈ N × 296 bytes

metadata (MiniKV)   = N × avg_meta_size × 1.2 (LSM overhead)
                    ≈ N × 100 bytes (typical)
```

Example — 1M vectors, 768-dim with PQ (M=96):
```
vector_data:  1M × 96     = 96 MB
index:        1M × 296    = 296 MB
metadata:     1M × 100    = 100 MB
Total:                    ≈ 500 MB
```

Same without PQ: `1M × 768 × 4 = 3072 MB + 296 MB + 100 MB ≈ 3.5 GB`.

---

## Troubleshooting

| Error | Cause | Solution |
|-------|-------|----------|
| `undefined reference to minikv::DB::Open` | MiniKV not linked | Add `vendor/MiniKV/libminikv.a` to link line |
| `error: 'mmap' was not declared` | Not on Linux/macOS | Use WSL2 or Docker |
| `cannot open /tmp/lumendb_data/vectors.bin` | Permission denied | Use a writable directory |
| `std::bad_alloc` | Out of memory | Reduce dim, use PQ, or lower N |
| `pybind11 not found` | Python bindings not built | Run cmake with `-DENABLE_PYTHON=ON` |
| `ImportError: No module named lumendb._lumendb` | Bindings not installed | `cd python && pip install -e .` |
| `GIL scoped release assertion` | Thread-safety violation | Ensure numpy arrays are contiguous C-order |
| `Search returns fewer than k results` | ef_search too low or filter too restrictive | Increase `ef_search` or widen filter |

---

## Next Steps

- **API Reference**: [API_REFERENCE.md](API_REFERENCE.md) — Complete C++, HTTP, and Python API docs
- **Architecture**: [ARCHITECTURE.md](ARCHITECTURE.md) — Deep dive into design decisions
- **Interview Prep**: [INTERVIEW_QA.md](INTERVIEW_QA.md) — 80+ Q&A for technical interviews
- **Contributing**: [CONTRIBUTING.md](CONTRIBUTING.md) — How to contribute
