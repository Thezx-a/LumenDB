# LumenDB Architecture

## 1. Architecture Overview

LumenDB is a C++17 embedded vector database organized in six layers. From top to bottom:

```
┌────────────────────────────────────────────────────────────┐
│                    HTTP Server                              │
│  server.cpp — select()-based event loop, REST endpoints,   │
│  bearer auth, JSON request/response, /health /search       │
│  /insert /stats                                             │
├────────────────────────────────────────────────────────────┤
│                  Python Bindings                            │
│  bindings.cpp — pybind11 zero-copy numpy, GIL release,     │
│  LangChain VectorStore integration                          │
├────────────────────────────────────────────────────────────┤
│                  Collection API                             │
│  collection.cpp — orchestrates index, storage, quantizers, │
│  distance callbacks, filtered search with ef-widening      │
├───────────────────┬────────────────────────────────────────┤
│   HNSW Index      │         MiniKV Metadata Store          │
│   hnsw.cpp        │         (LSM-Tree)                     │
│   distance.h      │   document_store.cpp wraps MiniKV DB   │
│   (AVX2 SIMD)     │   Bloom Filter, WAL, SSTables          │
├───────────────────┼────────────────────────────────────────┤
│              Vector Store (mmap)                            │
│   vector_store.cpp — 64B header, ID-to-offset map,         │
│   MAP_SHARED, soft-delete, grow via ftruncate+remmap       │
├────────────────────────────────────────────────────────────┤
│              Quantization                                   │
│   pq.cpp — Product Quantization (k-means, ADC/SDC)         │
│   scalar.cpp — Scalar Quantization (int8 per-dim)          │
└────────────────────────────────────────────────────────────┘
```

The data directory layout on disk:

```
<data_dir>/
├── vectors.bin          # mmap'd vector store (header + ID map + float32 data)
└── docs/                # MiniKV LSM-Tree directory
    ├── WAL files
    ├── MemTable (in-memory)
    └── SSTable files
```

## 2. Component Deep-Dives

### 2.1 HNSW Index (`include/lumendb/index/hnsw.h`, `src/index/hnsw.cpp`)

**Responsibility**: Provide approximate nearest-neighbor search over float32 vectors via a multi-layer navigable small-world graph.

**Key Design Decisions**:
- **Simple neighbor selection**: Uses `selectNeighborsSimple` (greedy — picks the M closest candidates) rather than the heuristic variant. This trades ~2-3% recall for ~30% faster construction and simpler code. The heuristic variant (which prunes redundant edges by checking relative distances) is a documented improvement path.
- **Layer assignment**: `level = floor(-ln(uniform(0,1)) * 1/ln(M))`. With M=16, ~6.25% of nodes reach level ≥1, ~0.4% reach level ≥2. This exponential distribution creates a logarithmic-diameter "highway" graph.
- **Bidirectional edges**: Each insertion adds edges in both directions at every layer up to the node's assigned level. After inserting all edges, `pruneNeighbors` trims each node's neighbor list to `M` (upper layers) or `2M` (layer 0).
- **Single `shared_mutex`**: One mutex for the entire index — shared lock for reads (concurrent searches), exclusive lock for writes (single inserter). Per-layer locking was considered but rejected for simplicity given that layer 0 dominates search cost.

**Trade-offs**:
- **Pro**: Incremental insertion (no batch training), sub-millisecond search at 50K scale, naturally handles insert-order-independent graph quality.
- **Con**: Simple neighbor selection leaves recall on the table (heuristic selection would improve it), graph quality degrades if insert order is adversarially sorted, no built-in support for shrinking the index.

**File References**:
- `include/lumendb/index/hnsw.h:21-65` — HNSWNode, HNSWIndex class definition
- `src/index/hnsw.cpp:1-175` — insert, search, searchLayer, selectNeighborsSimple, pruneNeighbors
- `include/lumendb/index/distance.h:1-142` — AVX2 and scalar distance kernels

### 2.2 Distance Kernels (`include/lumendb/index/distance.h`)

**Responsibility**: Compute L2 squared, inner product, and cosine distance between float32 vectors, dispatched to SIMD at compile time.

**Key Design Decisions**:
- **Header-only dispatch**: `#ifdef __AVX2__` selects AVX2 paths; `#ifdef __ARM_NEON` reserves an ARM slot. No runtime dispatch — the binary is compiled once for the target CPU, keeping the call site branch-free.
- **AVX2 implementation**: 8-wide `_mm256_loadu_ps` loads (no alignment requirement), `_mm256_fmadd_ps` for fused multiply-add on L2 and IP. Cosine falls back to scalar because the final `1.0 - dot/sqrt(na*nb)` division cannot be vectorized efficiently — the norm computation would need to be done per-candidate during search, which dominates the SIMD gain on the dot product alone.
- **L2 uses squared distance** for comparison (`l2_squared`), skipping the `sqrt`. This is safe because `sqrt` is monotonic — ranking is preserved. The full `l2_distance` with `sqrt` is available for callers that need the true distance.

**Trade-offs**:
- **Pro**: Near-optimal throughput on AVX2 hardware (~10× over scalar for L2 on 768-dim), zero runtime dispatch overhead.
- **Con**: No runtime CPU feature detection — binary must be compiled targeting the deployment hardware. No AVX-512 path (market penetration and frequency-throttling concerns, documented in INTERVIEW_QA.md Q11). Cosine is scalar-only.

**File References**:
- `include/lumendb/index/distance.h:24-31` — l2_squared_scalar
- `include/lumendb/index/distance.h:35-42` — ip_scalar (negated for minimizing)
- `include/lumendb/index/distance.h:45-53` — cosine_scalar
- `include/lumendb/index/distance.h:56-78` — l2_squared_avx2
- `include/lumendb/index/distance.h:80-95` — ip_avx2
- `include/lumendb/index/distance.h:103-135` — public dispatch functions

### 2.3 Vector Store (`include/lumendb/storage/vector_store.h`, `src/storage/vector_store.cpp`)

**Responsibility**: Persist float32 vectors to disk with zero-copy access via mmap.

**Key Design Decisions**:
- **Custom binary format** (not Arrow): 64-byte header (magic `0x4C554D454E444220` = "LUMENDB ", dimension, count, capacity, data_offset, id_offset) followed by an ID-to-slot mapping array (capacity × 8B uint64_t) and raw float32 vector data (capacity × dim × 4B). Simpler than embedding an Arrow dependency; sufficient for the single-producer, append-mostly workload.
- **`MAP_SHARED` + `msync(MS_SYNC)`**: Writes go through the OS page cache and are visible to other processes mapping the same file. `msync(MS_SYNC)` provides synchronous durability equivalent to `fsync`. The OS manages eviction transparently.
- **Soft-delete**: `remove(id)` sets the ID slot to `kInvalidID` (0) and decrements `count_`. The vector data is not zeroed; the slot is reused by `nextID()`. No compaction pass exists yet.
- **Grow strategy**: When capacity is exceeded, `grow(new_capacity)` calls `msync → munmap → ftruncate → mmap` to extend the file. This invalidates all existing pointers into the mmap region — callers must not hold `get()` results across inserts that could trigger growth.

**Trade-offs**:
- **Pro**: Instant restart (pages faulted in lazily, not deserialized), zero-copy access from C++ and Python, OS handles page cache eviction under memory pressure.
- **Con**: No compaction (free slots accumulate until reused), growth is stop-the-world (concurrent readers would see stale pointers), `msync(MS_SYNC)` blocks on every `flush()` call.

**File References**:
- `src/storage/vector_store.cpp:17-22` — header layout constants
- `src/storage/vector_store.cpp:32-59` — constructor, file creation, mmap
- `src/storage/vector_store.cpp:83-113` — nextID, ensureCapacity, grow
- `src/storage/vector_store.cpp:115-131` — append
- `src/storage/vector_store.cpp:133-140` — get
- `src/storage/vector_store.cpp:142-150` — remove (soft-delete)
- `src/storage/vector_store.cpp:152-185` — load from existing file

### 2.4 Document Store / Metadata (`include/lumendb/storage/document_store.h`, `src/storage/document_store.cpp`)

**Responsibility**: Store and retrieve per-vector metadata (text, tags, timestamp) using an embedded LSM-Tree (MiniKV).

**Key Design Decisions**:
- **PIMPL wrapper around MiniKV**: Isolates LSM-Tree internals from the Collection API. The wrapper stores `DocumentMeta` as length-prefixed binary blobs keyed by uint64 vector ID (big-endian for sort order).
- **Key-value model**: One MiniKV entry per vector. Filter evaluation fetches metadata by ID via the `FieldAccessor` callback, which calls `docs_->get(id)` → MiniKV point lookup.
- **No secondary indexing**: Metadata filtering is post-filter — the HNSW search returns candidates, then each candidate's metadata is fetched and evaluated. This means filter selectivity directly impacts latency: a filter matching 1% of vectors requires searching ~100× more candidates than k.

**Trade-offs**:
- **Pro**: Simple integration (MiniKV is a vendored dependency), WAL durability for metadata, Bloom filters accelerate point lookups.
- **Con**: Post-filter inefficiency for restrictive filters (see filtered search path below), no range scans on metadata fields, no schema enforcement.

**File References**:
- `include/lumendb/storage/document_store.h` — DocumentMeta struct and DocumentStore class
- `src/storage/document_store.cpp` — PIMPL wrapping MiniKV

### 2.5 Quantization (`include/lumendb/quantize/pq.h`, `include/lumendb/quantize/scalar.h`)

**Responsibility**: Compress vectors for reduced memory footprint and faster approximate distance computation.

**Product Quantizer** (`src/quantize/pq.cpp`):
- Splits `dim`-dimensional vectors into `M` subspaces of `dsub = dim/M` dimensions each.
- Trains `M` independent k-means codebooks (K=256 centroids each, fitting in uint8 codes).
- Encoding: `dim × 4` bytes → `M` bytes (e.g., 768 × 4 = 3072B → 96B for M=96 → 32× compression).
- Supports both ADC (query not quantized, distance table precomputed per query) and SDC (both query and candidate quantized).
- Training: random initialization (fixed seed 42), Lloyd iteration (max 25 rounds) per subspace. No k-means++ — random init with more iterations is simpler and converges to comparable quality on typical embedding data.
- `batchADC`: precompute distance table once (`O(M × K × dsub)`), evaluate n codes at `O(n × M)` via table lookups.

**Scalar Quantizer** (`src/quantize/scalar.cpp`):
- Per-dimension linear scaling: `code[i] = round((v[i] - min[i]) / scale[i])`, clamped to `[-128, 127]`.
- `dim × 4` bytes → `dim × 1` byte (4× compression).
- L2 distance on quantized codes uses stored scale factors: `sum((code_a[i] - code_b[i])^2 * scale[i]^2)`.

**Trade-offs**:
- **Pro**: PQ provides extreme compression (32×) with acceptable recall loss (~2.5% at k=10), SQ provides moderate compression (4×) with near-zero recall loss, both integrate transparently via the distance callback pattern in Collection.
- **Con**: PQ training requires at least `kPQTrainThreshold=256` vectors accumulated before engaging, training is O(n × M × K × dsub × iterations), codebooks are static after training (no incremental updates), random k-means initialization may converge to poor local minima on some datasets.

**File References**:
- `include/lumendb/quantize/pq.h:1-75` — ProductQuantizer API
- `src/quantize/pq.cpp` — k-means training, encode/decode, ADC/SDC, batchADC
- `include/lumendb/quantize/scalar.h:1-28` — ScalarQuantizer API
- `src/quantize/scalar.cpp` — min/max training, encode/decode, l2SquaredDistance

### 2.6 HTTP Server (`include/lumendb/server/server.h`, `src/server/server.cpp`, `src/server/main.cpp`)

**Responsibility**: Expose Collection operations over HTTP/1.1 with JSON bodies.

**Key Design Decisions**:
- **`select()`-based event loop**: Single-threaded, non-blocking sockets, fd_set polling with 1-second timeout. Deliberately simple — for <1000 concurrent connections, `select()`'s O(n) fd_set scan is negligible compared to the O(dim) distance computations happening inside handlers.
- **Hand-rolled HTTP parser**: Minimal parsing extracts method, path, body, and `Authorization` header. No chunked encoding, no header continuation, no request pipelining — sufficient for the JSON-based RPC-over-HTTP interaction pattern.
- **PIMPL for POSIX isolation**: `LumenDBServer::Impl` hides `<sys/socket.h>`, `<netinet/in.h>`, `fcntl.h` etc. from the public header. Consumers see only standard-library types.
- **Bearer token auth**: `Authorization: Bearer <key>` checked on every non-`/health` request. Empty API key disables auth. Keys are stored in plaintext in `ServerConfig` (no hashing).

**Endpoints**:
| Method | Path | Auth | Description |
|--------|------|------|-------------|
| GET | /health | No | Liveness: vector count, dimension |
| GET | /stats | Yes | Atomic counters (requests, searches, inserts, errors, connections) |
| POST | /search | Yes | k-NN search with optional JSON filter tree |
| POST | /insert | Yes | Single or batch vector insertion |
| DELETE | /vectors/:id | Yes | Soft-delete by ID |

**Trade-offs**:
- **Pro**: Minimal dependency footprint, single-threaded eliminates data races at server layer, PIMPL enables swapping `select()` for epoll without API changes.
- **Con**: `select()` scalability limit (FD_SETSIZE=1024 on most platforms), no keep-alive despite sending the header (each connection is one request then closed), no TLS, no request pipelining, no streaming responses, plaintext API keys in memory.

**File References**:
- `include/lumendb/server/server.h:1-46` — ServerConfig, ServerStats, LumenDBServer
- `src/server/server.cpp:1-290` — Impl, event loop, request handling, filter building
- `src/server/main.cpp` — CLI argument parsing, signal handling, server startup

## 3. Data Flows

### 3.1 Insert Path

```
Client → HTTP POST /insert → server::handleRequest()
  → json::parse(body)
  → Collection::add(vector)
    → if pq_/sq_ enabled: train_data_.push_back(vector)
    → if train_data_.size() >= threshold: trainQuantizers()
    → store_->append(vector)          # mmap write + header update
    → index_->insert(id)              # HNSW graph insertion
      → randomLevel()                 # assign layer
      → unique_lock<shared_mutex>     # exclusive write access
      → for each layer lc descending:
          → searchLayer(lc, ef_construction)  # find neighbors
          → selectNeighborsSimple(M)          # pick M closest
          → add bidirectional edges
      → for each layer lc:
          → pruneNeighbors for all connected nodes  # enforce M/2M
    → return id
```

Key points:
- Quantizer training is lazy — data accumulates in `train_data_` (an in-memory float vector) until `kPQTrainThreshold=256` or `kSQTrainThreshold=100` vectors exist, then k-means runs once.
- The HNSW insert temporarily swaps `query_dist_` with a custom lambda that uses `pairwise_dist_` (this is how insert finds neighbors using the existing graph's distance function without re-encoding the new vector).
- `searchLayer` at `ef=1` is used for greedy descent at upper layers; `ef_construction=200` at the target layer.

### 3.2 Search Path

```
Client → HTTP POST /search → server::handleRequest()
  → json::parse(body) → extract vector, k
  → Collection::search(query, k)
    → shared_lock<shared_mutex>              # concurrent reads
    → entry_point_ → greedy descent layers max_level...1 (ef=1)
    → searchLayer(layer 0, ef_search=50)     # beam search
      → priority_queue (candidates, min-heap)
      → priority_queue (results, max-heap, capped at ef)
      → while candidates not empty:
          → pop nearest candidate
          → if candidate.dist > worst(result): break  # pruning
          → for each neighbor:
              → if not visited: compute distance, push to both queues
    → collect top-k non-deleted candidates
    → return SearchResult{id, distance}[]
```

Key points:
- `query_dist_` is set by Collection to either raw, PQ, or SQ distance based on runtime state.
- The beam search at layer 0 is the dominant cost — it explores `ef_search × M` candidates at ~100ns per distance computation (AVX2, 128-dim).
- Deleted nodes are filtered during final collection, not during traversal (they remain in neighbor lists to preserve graph connectivity).

### 3.3 Filtered Search Path

```
Client → HTTP POST /search + filter JSON → server::handleRequest()
  → buildFilter(json) → FilterNode AST  # tree of eq/gt/lt/and/or/not
  → Collection::searchWithFilter(query, k, filter)
    → loop: progressively widen ef_search (×20 per attempt, max 10 attempts)
    → HNSW search with widened ef → candidates
    → for each candidate:
        → evaluateFilter(filter, id, fieldAccessor)
          → fieldAccessor(id, field):
              → docs_->get(id) → DocumentMeta  # MiniKV point lookup
              → extract field by name (text, tags, timestamp)
          → FilterOp switch: eq/gt/lt/contains/and/or/not
        → if match: push to results
    → stop when results.size() >= k or candidates exhausted
    → restore original ef_search
```

Key points:
- This is a **post-filter** strategy: the ANN search runs first, then metadata is checked. For highly selective filters (e.g., matching 1% of data), this requires scanning ~100× more candidates than k.
- The progressive EF-widening strategy: start with `ef_search=20`, then 40, 60, etc. Avoids scanning the entire dataset for every filtered query.
- Each filter evaluation triggers a MiniKV point lookup → Bloom filter check → potential SSTable read. This I/O is inside the hot loop — filtered search latency is dominated by metadata access, not distance computation.

### 3.4 Quantized Distance Path

```
Collection constructor:
  → setPairwiseDistance(lambda):
      if pq_ trained: encode(a) → encode(b) → pq_->symmetricDistance()
      elif sq_ trained: encode(a) → encode(b) → sq_->l2SquaredDistance()
      else: compute_distance(a, b, dim, metric) → AVX2 or scalar

  → setQueryDistance(lambda):
      if pq_ trained: pq_->computeDistanceTable(query, table) [O(M*K*dsub)]
                      encode(candidate) → pq_->asymmetricDistance(codes, table) [O(M)]
      elif sq_ trained: encode(query) → encode(candidate) → sq_->l2SquaredDistance()
      else: compute_distance(candidate, query, dim, metric)
```

Key points:
- The distance table for PQ is computed once per query (not per candidate), amortizing the `O(M×K×dsub)` cost across all candidates.
- SDC (symmetric) is used for `pairwise_dist_` (graph construction/insert), ADC (asymmetric) for `query_dist_` (search). The query vector is not quantized in ADC, reducing quantization error.

## 4. On-Disk Format

### 4.1 Vector Store (`vectors.bin`)

```
Offset  Size    Field           Description
─────────────────────────────────────────────────────
0       8B      magic           uint64, 0x4C554D454E444220 ("LUMENDB ")
8       4B      dim             uint32, vector dimension
12      4B      count           uint32, number of active vectors
16      4B      capacity        uint32, total slots allocated
20      4B      data_offset     uint32, byte offset to vector data region
24      4B      id_offset       uint32, byte offset to ID mapping region
28      36B     reserved        padding to 64B header boundary
64      N×8B    ID map          uint64_t per slot: valid ID or 0 (free)
                                where N = capacity
64+N×8  M×4B    vector data     float32, row-major: M = capacity × dim
                                vector for slot i starts at:
                                data_offset + i × dim × sizeof(float)
```

Header fields are little-endian on x86. The magic number `0x4C554D454E444220` encodes "LUMENDB " in ASCII (note trailing space for 8-byte alignment).

### 4.2 Document Store Metadata (`docs/`)

Uses MiniKV's native LSM-Tree format:
- **Key**: 8-byte big-endian uint64 (vector ID) — big-endian for lexicographic sort order matching numeric order.
- **Value**: Binary `DocumentMeta` serialized as length-prefixed fields:
  ```
  [text_len:4B][text:variable][tags_len:4B][tags:variable][timestamp:8B]
  ```
- **WAL**: Append-only write-ahead log for durability.
- **SSTables**: Sorted immutable files organized in levels (Level 0 can overlap; Level 1+ are disjoint).
- **Bloom filter**: Per SSTable, ~1% false-positive rate, accelerates point lookups.

## 5. Concurrency Model

| Component | Mechanism | Details |
|-----------|-----------|---------|
| HNSW Index | `std::shared_mutex` | Multiple concurrent `search()` (shared), exclusive `insert()` (unique). `ef_search_` is mutable for filtered search widening. `rng_` is mutable for `randomLevel()` during insert. |
| Vector Store | No internal locks | Single-writer assumption. Concurrent reads of mmap'd data are safe (kernel page cache handles coherence). Grow invalidates pointers — must not race with readers. |
| Document Store | MiniKV internal | MiniKV handles its own locking for MemTable, WAL, and SSTable access. |
| HTTP Server | Single-threaded | One `select()` loop, serialized request handling. No data races at server layer. |
| Python GIL | Released during C++ calls | `py::gil_scoped_release` before compute-heavy operations. Python threads can run concurrently with C++ work. |

**Safety properties**:
- Search-only workloads: fully concurrent, bounded only by CPU cores.
- Mixed read/write: readers block during insert (unique lock), but insert is fast (~30µs at 128-dim) — contention is low unless insert rate is very high.
- Filtered search: the `const_cast` + ef_search mutation is safe because `ef_search_` is mutable and the operation restores the original value before returning.

## 6. Error Handling Strategy

| Layer | Approach | Example |
|-------|----------|---------|
| VectorStore | Exceptions on fatal errors (mmap failure, file creation) | `throw std::runtime_error("mmap failed")` |
| HNSW Index | Silent guards — skip invalid IDs, return empty results | `if (id >= nodes_.size()) continue;` |
| Collection | Propagates store/index errors; returns `nullptr`/`nullopt` for missing data | `getVector` returns `nullptr` for invalid/deleted IDs |
| HTTP Server | Catch `std::exception`, return 400 with error message | `catch (const std::exception& e) { ... }` |
| Distance callbacks | Return `numeric_limits<float>::max()` for invalid comparisons | `if (!va || !vb) return FLT_MAX;` |
| Filter evaluation | Default to false for unknown ops/missing fields | `default: return false;` |

**Limitations**:
- No error codes — exceptions are the primary error propagation mechanism. This is acceptable for an embedded library but would need structured error types for a production API.
- No retry logic — mmap or I/O failures are fatal. A production system would retry transient errors (ENOMEM, EINTR).
- `Collection::load` is a stub returning nullptr. Persistence requires restarting from an existing data directory.

## 7. Testing Strategy

| Test File | What It Tests | Strategy |
|-----------|---------------|----------|
| `test_distance.cpp` | L2, IP, Cosine for AVX2 and scalar paths | Correctness: verify distance values against known vectors. Test zero vectors, identical vectors, orthogonal vectors. |
| `test_hnsw.cpp` | Insert, search, delete, recall@k | Correctness: verify search returns inserted vectors. Verify delete excludes vectors. Recall: measure % of true nearest neighbors found (target >95%). |
| `test_collection.cpp` | Collection API, storage round-trip | Integration: add → search → verify. Verify vector retrieval matches. |
| `test_filter.cpp` | FilterNode evaluation, edge cases | Correctness: eq/gt/lt/contains/and/or/not with string and numeric comparisons. Empty fields, missing values. |
| `test_collection_filter.cpp` | Filtered search end-to-end | Integration: add with metadata → filtered search → verify matching IDs. |
| `test_quantize.cpp` | PQ/SQ training, encode/decode, distance accuracy | Correctness: encode → decode → measure reconstruction error. Verify ADC/SDC produce consistent results. |

Tests are built with GoogleTest and discovered via `gtest_discover_tests`. Run with:
```bash
cmake -B build -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

**What's not tested**:
- Concurrency stress tests (multi-threaded search while inserting)
- HTTP server integration tests (requires network setup)
- Performance regression tests (benchmarks are separate)
- Memory leak detection (requires valgrind/ASAN integration)
- File corruption recovery (truncated vectors.bin, partial MiniKV WAL)

## 8. Performance Characteristics

### 8.1 Raw Float32 (128-dim, 50K vectors, g++-12, AVX2)

Measured by `benchmarks/bench_hnsw.cpp` (L2 metric, M=16, ef_construction=200, ef_search=50):

| Metric | Value |
|--------|-------|
| Insert throughput | ~30K vectors/second |
| Search P50 latency | ~150µs |
| Search P99 latency | ~800µs |
| Memory (vectors only) | dim × 4B ≈ 512B/vector |
| Memory (index overhead) | ~2M × sizeof(HNSWNode) ≈ ~200B/vector |

### 8.2 With Product Quantization (estimates, PQ M=dim/4=32, K=256)

| Metric | Value |
|--------|-------|
| Insert throughput | ~35K vectors/second (less distance computation) |
| Search P50 latency | ~80µs (table lookup vs full distance) |
| Search P99 latency | ~400µs |
| Memory (vectors) | dim/32 × 1B = 4B/vector (32× compression) |
| Recall@10 | ~97% (vs 99.5% raw) |

### 8.3 How to Reproduce

```bash
# Benchmark raw float32
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_BENCHMARKS=ON -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
./build/benchmarks/bench_hnsw
```

To test PQ: compile with PQ enabled, modify `bench_hnsw.cpp` to set `config.use_pq = true`, re-run.

### 8.4 Scaling Projections

HNSW search complexity is O(log N × M) for greedy descent plus O(ef_search × M) at layer 0. At 50K vectors:
- Layer 0 is the bottleneck — ~50K × M = 800 distance computations at ef_search=50.
- At 1M vectors (20× more): ~20× more candidates at layer 0 → ~20× latency (~3ms P50, ~16ms P99) assuming same ef_search.
- Mitigation: increase ef_search for recall, use PQ for faster distance computation, shard across nodes.

## 9. Known Limitations & Roadmap

### Current Limitations

1. **HNSW simple neighbor selection**: Uses greedy M-closest selection. Heuristic selection (pruning redundant edges) would improve recall by 2-3% at the same ef_search. Documented in `hnsw.cpp:37-41`.

2. **`select()` event loop**: FD_SETSIZE limited, O(n) scanning. Planned migration to epoll (Linux) / kqueue (macOS) via the SkyNet vendor library. See `server.cpp:110-150`.

3. **No connection keep-alive**: Server sends `Connection: keep-alive` in responses but closes the socket after one request. See `server.cpp:145-170`.

4. **Post-filter only**: All metadata filtering happens after ANN search. For highly selective filters (1% match rate), 99% of distance computations are wasted. A pre-filter or hybrid approach would help.

5. **No vector store compaction**: Soft-deleted slots accumulate until reused. A background compaction thread could reclaim contiguous free ranges.

6. **Collection::load is a stub**: Returns nullptr. Restart works by constructing a Collection with an existing data_dir (VectorStore auto-detects existing files), but the `load`/`save` API is not functional.

7. **Plaintext API keys**: Stored in `ServerConfig::api_key` as a `std::string`. No hashing, no key rotation.

8. **Grow stops the world**: VectorStore growth invalidates all mmap pointers. No double-buffering or segmented mapping.

9. **PQ training is one-shot**: Once trained, codebooks are never updated. Data distribution drift degrades quantization quality over time.

10. **No TLS/SSL**: All traffic is plain HTTP. For production, put a reverse proxy (nginx, envoy) in front.

### Roadmap

| Feature | Priority | Effort | Notes |
|---------|----------|--------|-------|
| epoll/kqueue migration | High | Medium | Replace `select()` in server event loop |
| Connection keep-alive | High | Small | Buffer management per connection |
| Heuristic neighbor selection | Medium | Medium | Replace `selectNeighborsSimple` |
| HNSW save/load | Medium | Medium | Serialize graph to disk, restore without rebuild |
| VectorStore compaction | Medium | Medium | Background thread, reclaim free slots |
| Prometheus /metrics endpoint | Medium | Small | Leverage existing atomic counters |
| TLS via OpenSSL | Low | Medium | Or document reverse-proxy pattern |
| Pre-filter support | Low | Large | Requires secondary index in MiniKV |
| gRPC API | Low | Large | Alternative to REST/JSON |
| Distributed sharding | Low | Very Large | Consistent hashing, Raft consensus |

## 10. Deployment

### 10.1 Docker Topology

```
┌──────────────────┐
│   Client (curl)   │
└────────┬─────────┘
         │ HTTP :8080
┌────────▼─────────┐
│  lumendb_server   │  Docker container
│  ┌─────────────┐ │
│  │ select()     │ │  Single-threaded
│  │ event loop  │ │
│  └──────┬──────┘ │
│         │         │
│  ┌──────▼──────┐ │
│  │ Collection  │ │  HNSW + MiniKV
│  └──────┬──────┘ │
│         │         │
│  ┌──────▼──────┐ │
│  │ /data volume│ │  vectors.bin + docs/
│  └─────────────┘ │
└──────────────────┘
```

### 10.2 Resource Requirements

| Scale | Vectors (128-dim) | RAM (raw) | RAM (PQ) | Disk |
|-------|-------------------|-----------|----------|------|
| Small | 10K | ~5 MB | <1 MB | ~5 MB |
| Medium | 100K | ~50 MB | ~2 MB | ~50 MB |
| Large | 1M | ~500 MB | ~16 MB | ~500 MB |
| Max (PQ) | 10M | — | ~160 MB | ~5 GB |

RAM includes vector data + HNSW index overhead (~200B/node for M=16). With PQ, vector data is negligible; index overhead dominates.

### 10.3 Docker Compose

```yaml
# docker-compose.yml
version: "3.8"
services:
  lumendb:
    build: .
    ports:
      - "8080:8080"
    volumes:
      - ./data:/data
    environment:
      - LUMENDB_DIM=768
      - LUMENDB_PORT=8080
    restart: unless-stopped
```

The Dockerfile uses multi-stage builds: g++-12 + CMake in the builder stage, then copies the statically-linked `lumendb_server` binary to a minimal Ubuntu 22.04 runtime image. Final image is <100MB.

## 11. Security Model

### Current State

| Concern | Implementation | Gap |
|---------|---------------|-----|
| Authentication | Bearer token (`Authorization: Bearer <key>`) | Plaintext key in config, no key rotation |
| Authorization | Single key for all endpoints | No per-endpoint or per-collection ACLs |
| Transport | Plain HTTP/1.1 | No TLS — use reverse proxy |
| Input validation | JSON parse exception handled | No size limits on vectors or batch size |
| Rate limiting | None | No protection against DoS |
| Memory safety | C++ with manual pointer management in mmap regions | No bounds checking on ID-to-offset mapping |
| Secrets management | API key in command-line args | Visible in `ps`, no secrets manager integration |

### Recommendations for Production

1. **TLS termination**: Place nginx, envoy, or haproxy in front of LumenDB.
2. **API key hashing**: Store `SHA256(key)` in config, compare hashes.
3. **Rate limiting**: Token bucket per API key at the reverse proxy or in-server.
4. **Input bounds**: Reject vectors with `dim` mismatch, limit batch insert size.
5. **Non-root user**: The Docker image runs as root; add `USER lumendb` with dropped capabilities.
6. **Read-only data dir**: After initial population, mount `/data` read-only and disable insert endpoints.

---

For deeper algorithmic questions about HNSW, SIMD, LSM-trees, quantization, and comparison to other vector databases, see [INTERVIEW_QA.md](INTERVIEW_QA.md).
