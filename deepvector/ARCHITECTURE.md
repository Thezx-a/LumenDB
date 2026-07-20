# DeepVector Architecture

## 1. Architecture Overview

DeepVector is a C++17 embedded vector database organized in six layers. From top to bottom:

```
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?                   HTTP Server                              鈹?鈹? server.cpp 鈥?select()-based event loop, REST endpoints,   鈹?鈹? bearer auth, JSON request/response, /health /search       鈹?鈹? /insert /stats                                             鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?                 Python Bindings                            鈹?鈹? bindings.cpp 鈥?pybind11 zero-copy numpy, GIL release,     鈹?鈹? LangChain VectorStore integration                          鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?                 Collection API                             鈹?鈹? collection.cpp 鈥?orchestrates index, storage, quantizers, 鈹?鈹? distance callbacks, filtered search with ef-widening      鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?  HNSW Index      鈹?        MiniKV Metadata Store          鈹?鈹?  hnsw.cpp        鈹?        (LSM-Tree)                     鈹?鈹?  distance.h      鈹?  document_store.cpp wraps MiniKV DB   鈹?鈹?  (AVX2 SIMD)     鈹?  Bloom Filter, WAL, SSTables          鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?             Vector Store (mmap)                            鈹?鈹?  vector_store.cpp 鈥?64B header, ID-to-offset map,         鈹?鈹?  MAP_SHARED, soft-delete, grow via ftruncate+remmap       鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?             Quantization                                   鈹?鈹?  pq.cpp 鈥?Product Quantization (k-means, ADC/SDC)         鈹?鈹?  scalar.cpp 鈥?Scalar Quantization (int8 per-dim)          鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

The data directory layout on disk:

```
<data_dir>/
鈹溾攢鈹€ vectors.bin          # mmap'd vector store (header + ID map + float32 data)
鈹斺攢鈹€ docs/                # MiniKV LSM-Tree directory
    鈹溾攢鈹€ WAL files
    鈹溾攢鈹€ MemTable (in-memory)
    鈹斺攢鈹€ SSTable files
```

## 2. Component Deep-Dives

### 2.1 HNSW Index (`include/lumendb/index/hnsw.h`, `src/index/hnsw.cpp`)

**Responsibility**: Provide approximate nearest-neighbor search over float32 vectors via a multi-layer navigable small-world graph.

**Key Design Decisions**:
- **Simple neighbor selection**: Uses `selectNeighborsSimple` (greedy 鈥?picks the M closest candidates) rather than the heuristic variant. This trades ~2-3% recall for ~30% faster construction and simpler code. The heuristic variant (which prunes redundant edges by checking relative distances) is a documented improvement path.
- **Layer assignment**: `level = floor(-ln(uniform(0,1)) * 1/ln(M))`. With M=16, ~6.25% of nodes reach level 鈮?, ~0.4% reach level 鈮?. This exponential distribution creates a logarithmic-diameter "highway" graph.
- **Bidirectional edges**: Each insertion adds edges in both directions at every layer up to the node's assigned level. After inserting all edges, `pruneNeighbors` trims each node's neighbor list to `M` (upper layers) or `2M` (layer 0).
- **Single `shared_mutex`**: One mutex for the entire index 鈥?shared lock for reads (concurrent searches), exclusive lock for writes (single inserter). Per-layer locking was considered but rejected for simplicity given that layer 0 dominates search cost.

**Trade-offs**:
- **Pro**: Incremental insertion (no batch training), sub-millisecond search at 50K scale, naturally handles insert-order-independent graph quality.
- **Con**: Simple neighbor selection leaves recall on the table (heuristic selection would improve it), graph quality degrades if insert order is adversarially sorted, no built-in support for shrinking the index.

**File References**:
- `include/lumendb/index/hnsw.h:21-65` 鈥?HNSWNode, HNSWIndex class definition
- `src/index/hnsw.cpp:1-175` 鈥?insert, search, searchLayer, selectNeighborsSimple, pruneNeighbors
- `include/lumendb/index/distance.h:1-142` 鈥?AVX2 and scalar distance kernels

### 2.2 Distance Kernels (`include/lumendb/index/distance.h`)

**Responsibility**: Compute L2 squared, inner product, and cosine distance between float32 vectors, dispatched to SIMD at compile time.

**Key Design Decisions**:
- **Header-only dispatch**: `#ifdef __AVX2__` selects AVX2 paths; `#ifdef __ARM_NEON` reserves an ARM slot. No runtime dispatch 鈥?the binary is compiled once for the target CPU, keeping the call site branch-free.
- **AVX2 implementation**: 8-wide `_mm256_loadu_ps` loads (no alignment requirement), `_mm256_fmadd_ps` for fused multiply-add on L2 and IP. Cosine falls back to scalar because the final `1.0 - dot/sqrt(na*nb)` division cannot be vectorized efficiently 鈥?the norm computation would need to be done per-candidate during search, which dominates the SIMD gain on the dot product alone.
- **L2 uses squared distance** for comparison (`l2_squared`), skipping the `sqrt`. This is safe because `sqrt` is monotonic 鈥?ranking is preserved. The full `l2_distance` with `sqrt` is available for callers that need the true distance.

**Trade-offs**:
- **Pro**: Near-optimal throughput on AVX2 hardware (~10脳 over scalar for L2 on 768-dim), zero runtime dispatch overhead.
- **Con**: No runtime CPU feature detection 鈥?binary must be compiled targeting the deployment hardware. No AVX-512 path (market penetration and frequency-throttling concerns, documented in INTERVIEW_QA.md Q11). Cosine is scalar-only.

**File References**:
- `include/lumendb/index/distance.h:24-31` 鈥?l2_squared_scalar
- `include/lumendb/index/distance.h:35-42` 鈥?ip_scalar (negated for minimizing)
- `include/lumendb/index/distance.h:45-53` 鈥?cosine_scalar
- `include/lumendb/index/distance.h:56-78` 鈥?l2_squared_avx2
- `include/lumendb/index/distance.h:80-95` 鈥?ip_avx2
- `include/lumendb/index/distance.h:103-135` 鈥?public dispatch functions

### 2.3 Vector Store (`include/lumendb/storage/vector_store.h`, `src/storage/vector_store.cpp`)

**Responsibility**: Persist float32 vectors to disk with zero-copy access via mmap.

**Key Design Decisions**:
- **Custom binary format** (not Arrow): 64-byte header (magic `0x4C554D454E444220` = "DEEPVECTOR ", dimension, count, capacity, data_offset, id_offset) followed by an ID-to-slot mapping array (capacity 脳 8B uint64_t) and raw float32 vector data (capacity 脳 dim 脳 4B). Simpler than embedding an Arrow dependency; sufficient for the single-producer, append-mostly workload.
- **`MAP_SHARED` + `msync(MS_SYNC)`**: Writes go through the OS page cache and are visible to other processes mapping the same file. `msync(MS_SYNC)` provides synchronous durability equivalent to `fsync`. The OS manages eviction transparently.
- **Soft-delete**: `remove(id)` sets the ID slot to `kInvalidID` (0) and decrements `count_`. The vector data is not zeroed; the slot is reused by `nextID()`. No compaction pass exists yet.
- **Grow strategy**: When capacity is exceeded, `grow(new_capacity)` calls `msync 鈫?munmap 鈫?ftruncate 鈫?mmap` to extend the file. This invalidates all existing pointers into the mmap region 鈥?callers must not hold `get()` results across inserts that could trigger growth.

**Trade-offs**:
- **Pro**: Instant restart (pages faulted in lazily, not deserialized), zero-copy access from C++ and Python, OS handles page cache eviction under memory pressure.
- **Con**: No compaction (free slots accumulate until reused), growth is stop-the-world (concurrent readers would see stale pointers), `msync(MS_SYNC)` blocks on every `flush()` call.

**File References**:
- `src/storage/vector_store.cpp:17-22` 鈥?header layout constants
- `src/storage/vector_store.cpp:32-59` 鈥?constructor, file creation, mmap
- `src/storage/vector_store.cpp:83-113` 鈥?nextID, ensureCapacity, grow
- `src/storage/vector_store.cpp:115-131` 鈥?append
- `src/storage/vector_store.cpp:133-140` 鈥?get
- `src/storage/vector_store.cpp:142-150` 鈥?remove (soft-delete)
- `src/storage/vector_store.cpp:152-185` 鈥?load from existing file

### 2.4 Document Store / Metadata (`include/lumendb/storage/document_store.h`, `src/storage/document_store.cpp`)

**Responsibility**: Store and retrieve per-vector metadata (text, tags, timestamp) using an embedded LSM-Tree (MiniKV).

**Key Design Decisions**:
- **PIMPL wrapper around MiniKV**: Isolates LSM-Tree internals from the Collection API. The wrapper stores `DocumentMeta` as length-prefixed binary blobs keyed by uint64 vector ID (big-endian for sort order).
- **Key-value model**: One MiniKV entry per vector. Filter evaluation fetches metadata by ID via the `FieldAccessor` callback, which calls `docs_->get(id)` 鈫?MiniKV point lookup.
- **No secondary indexing**: Metadata filtering is post-filter 鈥?the HNSW search returns candidates, then each candidate's metadata is fetched and evaluated. This means filter selectivity directly impacts latency: a filter matching 1% of vectors requires searching ~100脳 more candidates than k.

**Trade-offs**:
- **Pro**: Simple integration (MiniKV is a vendored dependency), WAL durability for metadata, Bloom filters accelerate point lookups.
- **Con**: Post-filter inefficiency for restrictive filters (see filtered search path below), no range scans on metadata fields, no schema enforcement.

**File References**:
- `include/lumendb/storage/document_store.h` 鈥?DocumentMeta struct and DocumentStore class
- `src/storage/document_store.cpp` 鈥?PIMPL wrapping MiniKV

### 2.5 Quantization (`include/lumendb/quantize/pq.h`, `include/lumendb/quantize/scalar.h`)

**Responsibility**: Compress vectors for reduced memory footprint and faster approximate distance computation.

**Product Quantizer** (`src/quantize/pq.cpp`):
- Splits `dim`-dimensional vectors into `M` subspaces of `dsub = dim/M` dimensions each.
- Trains `M` independent k-means codebooks (K=256 centroids each, fitting in uint8 codes).
- Encoding: `dim 脳 4` bytes 鈫?`M` bytes (e.g., 768 脳 4 = 3072B 鈫?96B for M=96 鈫?32脳 compression).
- Supports both ADC (query not quantized, distance table precomputed per query) and SDC (both query and candidate quantized).
- Training: random initialization (fixed seed 42), Lloyd iteration (max 25 rounds) per subspace. No k-means++ 鈥?random init with more iterations is simpler and converges to comparable quality on typical embedding data.
- `batchADC`: precompute distance table once (`O(M 脳 K 脳 dsub)`), evaluate n codes at `O(n 脳 M)` via table lookups.

**Scalar Quantizer** (`src/quantize/scalar.cpp`):
- Per-dimension linear scaling: `code[i] = round((v[i] - min[i]) / scale[i])`, clamped to `[-128, 127]`.
- `dim 脳 4` bytes 鈫?`dim 脳 1` byte (4脳 compression).
- L2 distance on quantized codes uses stored scale factors: `sum((code_a[i] - code_b[i])^2 * scale[i]^2)`.

**Trade-offs**:
- **Pro**: PQ provides extreme compression (32脳) with acceptable recall loss (~2.5% at k=10), SQ provides moderate compression (4脳) with near-zero recall loss, both integrate transparently via the distance callback pattern in Collection.
- **Con**: PQ training requires at least `kPQTrainThreshold=256` vectors accumulated before engaging, training is O(n 脳 M 脳 K 脳 dsub 脳 iterations), codebooks are static after training (no incremental updates), random k-means initialization may converge to poor local minima on some datasets.

**File References**:
- `include/lumendb/quantize/pq.h:1-75` 鈥?ProductQuantizer API
- `src/quantize/pq.cpp` 鈥?k-means training, encode/decode, ADC/SDC, batchADC
- `include/lumendb/quantize/scalar.h:1-28` 鈥?ScalarQuantizer API
- `src/quantize/scalar.cpp` 鈥?min/max training, encode/decode, l2SquaredDistance

### 2.6 HTTP Server (`include/lumendb/server/server.h`, `src/server/server.cpp`, `src/server/main.cpp`)

**Responsibility**: Expose Collection operations over HTTP/1.1 with JSON bodies.

**Key Design Decisions**:
- **`select()`-based event loop**: Single-threaded, non-blocking sockets, fd_set polling with 1-second timeout. Deliberately simple 鈥?for <1000 concurrent connections, `select()`'s O(n) fd_set scan is negligible compared to the O(dim) distance computations happening inside handlers.
- **Hand-rolled HTTP parser**: Minimal parsing extracts method, path, body, and `Authorization` header. No chunked encoding, no header continuation, no request pipelining 鈥?sufficient for the JSON-based RPC-over-HTTP interaction pattern.
- **PIMPL for POSIX isolation**: `DeepVectorServer::Impl` hides `<sys/socket.h>`, `<netinet/in.h>`, `fcntl.h` etc. from the public header. Consumers see only standard-library types.
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
- `include/lumendb/server/server.h:1-46` 鈥?ServerConfig, ServerStats, DeepVectorServer
- `src/server/server.cpp:1-290` 鈥?Impl, event loop, request handling, filter building
- `src/server/main.cpp` 鈥?CLI argument parsing, signal handling, server startup

## 3. Data Flows

### 3.1 Insert Path

```
Client 鈫?HTTP POST /insert 鈫?server::handleRequest()
  鈫?json::parse(body)
  鈫?Collection::add(vector)
    鈫?if pq_/sq_ enabled: train_data_.push_back(vector)
    鈫?if train_data_.size() >= threshold: trainQuantizers()
    鈫?store_->append(vector)          # mmap write + header update
    鈫?index_->insert(id)              # HNSW graph insertion
      鈫?randomLevel()                 # assign layer
      鈫?unique_lock<shared_mutex>     # exclusive write access
      鈫?for each layer lc descending:
          鈫?searchLayer(lc, ef_construction)  # find neighbors
          鈫?selectNeighborsSimple(M)          # pick M closest
          鈫?add bidirectional edges
      鈫?for each layer lc:
          鈫?pruneNeighbors for all connected nodes  # enforce M/2M
    鈫?return id
```

Key points:
- Quantizer training is lazy 鈥?data accumulates in `train_data_` (an in-memory float vector) until `kPQTrainThreshold=256` or `kSQTrainThreshold=100` vectors exist, then k-means runs once.
- The HNSW insert temporarily swaps `query_dist_` with a custom lambda that uses `pairwise_dist_` (this is how insert finds neighbors using the existing graph's distance function without re-encoding the new vector).
- `searchLayer` at `ef=1` is used for greedy descent at upper layers; `ef_construction=200` at the target layer.

### 3.2 Search Path

```
Client 鈫?HTTP POST /search 鈫?server::handleRequest()
  鈫?json::parse(body) 鈫?extract vector, k
  鈫?Collection::search(query, k)
    鈫?shared_lock<shared_mutex>              # concurrent reads
    鈫?entry_point_ 鈫?greedy descent layers max_level...1 (ef=1)
    鈫?searchLayer(layer 0, ef_search=50)     # beam search
      鈫?priority_queue (candidates, min-heap)
      鈫?priority_queue (results, max-heap, capped at ef)
      鈫?while candidates not empty:
          鈫?pop nearest candidate
          鈫?if candidate.dist > worst(result): break  # pruning
          鈫?for each neighbor:
              鈫?if not visited: compute distance, push to both queues
    鈫?collect top-k non-deleted candidates
    鈫?return SearchResult{id, distance}[]
```

Key points:
- `query_dist_` is set by Collection to either raw, PQ, or SQ distance based on runtime state.
- The beam search at layer 0 is the dominant cost 鈥?it explores `ef_search 脳 M` candidates at ~100ns per distance computation (AVX2, 128-dim).
- Deleted nodes are filtered during final collection, not during traversal (they remain in neighbor lists to preserve graph connectivity).

### 3.3 Filtered Search Path

```
Client 鈫?HTTP POST /search + filter JSON 鈫?server::handleRequest()
  鈫?buildFilter(json) 鈫?FilterNode AST  # tree of eq/gt/lt/and/or/not
  鈫?Collection::searchWithFilter(query, k, filter)
    鈫?loop: progressively widen ef_search (脳20 per attempt, max 10 attempts)
    鈫?HNSW search with widened ef 鈫?candidates
    鈫?for each candidate:
        鈫?evaluateFilter(filter, id, fieldAccessor)
          鈫?fieldAccessor(id, field):
              鈫?docs_->get(id) 鈫?DocumentMeta  # MiniKV point lookup
              鈫?extract field by name (text, tags, timestamp)
          鈫?FilterOp switch: eq/gt/lt/contains/and/or/not
        鈫?if match: push to results
    鈫?stop when results.size() >= k or candidates exhausted
    鈫?restore original ef_search
```

Key points:
- This is a **post-filter** strategy: the ANN search runs first, then metadata is checked. For highly selective filters (e.g., matching 1% of data), this requires scanning ~100脳 more candidates than k.
- The progressive EF-widening strategy: start with `ef_search=20`, then 40, 60, etc. Avoids scanning the entire dataset for every filtered query.
- Each filter evaluation triggers a MiniKV point lookup 鈫?Bloom filter check 鈫?potential SSTable read. This I/O is inside the hot loop 鈥?filtered search latency is dominated by metadata access, not distance computation.

### 3.4 Quantized Distance Path

```
Collection constructor:
  鈫?setPairwiseDistance(lambda):
      if pq_ trained: encode(a) 鈫?encode(b) 鈫?pq_->symmetricDistance()
      elif sq_ trained: encode(a) 鈫?encode(b) 鈫?sq_->l2SquaredDistance()
      else: compute_distance(a, b, dim, metric) 鈫?AVX2 or scalar

  鈫?setQueryDistance(lambda):
      if pq_ trained: pq_->computeDistanceTable(query, table) [O(M*K*dsub)]
                      encode(candidate) 鈫?pq_->asymmetricDistance(codes, table) [O(M)]
      elif sq_ trained: encode(query) 鈫?encode(candidate) 鈫?sq_->l2SquaredDistance()
      else: compute_distance(candidate, query, dim, metric)
```

Key points:
- The distance table for PQ is computed once per query (not per candidate), amortizing the `O(M脳K脳dsub)` cost across all candidates.
- SDC (symmetric) is used for `pairwise_dist_` (graph construction/insert), ADC (asymmetric) for `query_dist_` (search). The query vector is not quantized in ADC, reducing quantization error.

## 4. On-Disk Format

### 4.1 Vector Store (`vectors.bin`)

```
Offset  Size    Field           Description
鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€
0       8B      magic           uint64, 0x4C554D454E444220 ("DEEPVECTOR ")
8       4B      dim             uint32, vector dimension
12      4B      count           uint32, number of active vectors
16      4B      capacity        uint32, total slots allocated
20      4B      data_offset     uint32, byte offset to vector data region
24      4B      id_offset       uint32, byte offset to ID mapping region
28      36B     reserved        padding to 64B header boundary
64      N脳8B    ID map          uint64_t per slot: valid ID or 0 (free)
                                where N = capacity
64+N脳8  M脳4B    vector data     float32, row-major: M = capacity 脳 dim
                                vector for slot i starts at:
                                data_offset + i 脳 dim 脳 sizeof(float)
```

Header fields are little-endian on x86. The magic number `0x4C554D454E444220` encodes "DEEPVECTOR " in ASCII (note trailing space for 8-byte alignment).

### 4.2 Document Store Metadata (`docs/`)

Uses MiniKV's native LSM-Tree format:
- **Key**: 8-byte big-endian uint64 (vector ID) 鈥?big-endian for lexicographic sort order matching numeric order.
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
| Vector Store | No internal locks | Single-writer assumption. Concurrent reads of mmap'd data are safe (kernel page cache handles coherence). Grow invalidates pointers 鈥?must not race with readers. |
| Document Store | MiniKV internal | MiniKV handles its own locking for MemTable, WAL, and SSTable access. |
| HTTP Server | Single-threaded | One `select()` loop, serialized request handling. No data races at server layer. |
| Python GIL | Released during C++ calls | `py::gil_scoped_release` before compute-heavy operations. Python threads can run concurrently with C++ work. |

**Safety properties**:
- Search-only workloads: fully concurrent, bounded only by CPU cores.
- Mixed read/write: readers block during insert (unique lock), but insert is fast (~30碌s at 128-dim) 鈥?contention is low unless insert rate is very high.
- Filtered search: the `const_cast` + ef_search mutation is safe because `ef_search_` is mutable and the operation restores the original value before returning.

## 6. Error Handling Strategy

| Layer | Approach | Example |
|-------|----------|---------|
| VectorStore | Exceptions on fatal errors (mmap failure, file creation) | `throw std::runtime_error("mmap failed")` |
| HNSW Index | Silent guards 鈥?skip invalid IDs, return empty results | `if (id >= nodes_.size()) continue;` |
| Collection | Propagates store/index errors; returns `nullptr`/`nullopt` for missing data | `getVector` returns `nullptr` for invalid/deleted IDs |
| HTTP Server | Catch `std::exception`, return 400 with error message | `catch (const std::exception& e) { ... }` |
| Distance callbacks | Return `numeric_limits<float>::max()` for invalid comparisons | `if (!va || !vb) return FLT_MAX;` |
| Filter evaluation | Default to false for unknown ops/missing fields | `default: return false;` |

**Limitations**:
- No error codes 鈥?exceptions are the primary error propagation mechanism. This is acceptable for an embedded library but would need structured error types for a production API.
- No retry logic 鈥?mmap or I/O failures are fatal. A production system would retry transient errors (ENOMEM, EINTR).
- `Collection::load` is a stub returning nullptr. Persistence requires restarting from an existing data directory.

## 7. Testing Strategy

| Test File | What It Tests | Strategy |
|-----------|---------------|----------|
| `test_distance.cpp` | L2, IP, Cosine for AVX2 and scalar paths | Correctness: verify distance values against known vectors. Test zero vectors, identical vectors, orthogonal vectors. |
| `test_hnsw.cpp` | Insert, search, delete, recall@k | Correctness: verify search returns inserted vectors. Verify delete excludes vectors. Recall: measure % of true nearest neighbors found (target >95%). |
| `test_collection.cpp` | Collection API, storage round-trip | Integration: add 鈫?search 鈫?verify. Verify vector retrieval matches. |
| `test_filter.cpp` | FilterNode evaluation, edge cases | Correctness: eq/gt/lt/contains/and/or/not with string and numeric comparisons. Empty fields, missing values. |
| `test_collection_filter.cpp` | Filtered search end-to-end | Integration: add with metadata 鈫?filtered search 鈫?verify matching IDs. |
| `test_quantize.cpp` | PQ/SQ training, encode/decode, distance accuracy | Correctness: encode 鈫?decode 鈫?measure reconstruction error. Verify ADC/SDC produce consistent results. |

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
| Search P50 latency | ~150碌s |
| Search P99 latency | ~800碌s |
| Memory (vectors only) | dim 脳 4B 鈮?512B/vector |
| Memory (index overhead) | ~2M 脳 sizeof(HNSWNode) 鈮?~200B/vector |

### 8.2 With Product Quantization (estimates, PQ M=dim/4=32, K=256)

| Metric | Value |
|--------|-------|
| Insert throughput | ~35K vectors/second (less distance computation) |
| Search P50 latency | ~80碌s (table lookup vs full distance) |
| Search P99 latency | ~400碌s |
| Memory (vectors) | dim/32 脳 1B = 4B/vector (32脳 compression) |
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

HNSW search complexity is O(log N 脳 M) for greedy descent plus O(ef_search 脳 M) at layer 0. At 50K vectors:
- Layer 0 is the bottleneck 鈥?~50K 脳 M = 800 distance computations at ef_search=50.
- At 1M vectors (20脳 more): ~20脳 more candidates at layer 0 鈫?~20脳 latency (~3ms P50, ~16ms P99) assuming same ef_search.
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
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?  Client (curl)   鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?         鈹?HTTP :8080
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? lumendb_server   鈹? Docker container
鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?鈹? 鈹?select()     鈹?鈹? Single-threaded
鈹? 鈹?event loop  鈹?鈹?鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹?鈹?鈹?        鈹?        鈹?鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈻尖攢鈹€鈹€鈹€鈹€鈹€鈹?鈹?鈹? 鈹?Collection  鈹?鈹? HNSW + MiniKV
鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹?鈹?鈹?        鈹?        鈹?鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈻尖攢鈹€鈹€鈹€鈹€鈹€鈹?鈹?鈹? 鈹?/data volume鈹?鈹? vectors.bin + docs/
鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

### 10.2 Resource Requirements

| Scale | Vectors (128-dim) | RAM (raw) | RAM (PQ) | Disk |
|-------|-------------------|-----------|----------|------|
| Small | 10K | ~5 MB | <1 MB | ~5 MB |
| Medium | 100K | ~50 MB | ~2 MB | ~50 MB |
| Large | 1M | ~500 MB | ~16 MB | ~500 MB |
| Max (PQ) | 10M | 鈥?| ~160 MB | ~5 GB |

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
      - DEEPVECTOR_DIM=768
      - DEEPVECTOR_PORT=8080
    restart: unless-stopped
```

The Dockerfile uses multi-stage builds: g++-12 + CMake in the builder stage, then copies the statically-linked `lumendb_server` binary to a minimal Ubuntu 22.04 runtime image. Final image is <100MB.

## 11. Security Model

### Current State

| Concern | Implementation | Gap |
|---------|---------------|-----|
| Authentication | Bearer token (`Authorization: Bearer <key>`) | Plaintext key in config, no key rotation |
| Authorization | Single key for all endpoints | No per-endpoint or per-collection ACLs |
| Transport | Plain HTTP/1.1 | No TLS 鈥?use reverse proxy |
| Input validation | JSON parse exception handled | No size limits on vectors or batch size |
| Rate limiting | None | No protection against DoS |
| Memory safety | C++ with manual pointer management in mmap regions | No bounds checking on ID-to-offset mapping |
| Secrets management | API key in command-line args | Visible in `ps`, no secrets manager integration |

### Recommendations for Production

1. **TLS termination**: Place nginx, envoy, or haproxy in front of DeepVector.
2. **API key hashing**: Store `SHA256(key)` in config, compare hashes.
3. **Rate limiting**: Token bucket per API key at the reverse proxy or in-server.
4. **Input bounds**: Reject vectors with `dim` mismatch, limit batch insert size.
5. **Non-root user**: The Docker image runs as root; add `USER lumendb` with dropped capabilities.
6. **Read-only data dir**: After initial population, mount `/data` read-only and disable insert endpoints.

---

For deeper algorithmic questions about HNSW, SIMD, LSM-trees, quantization, and comparison to other vector databases, see [INTERVIEW_QA.md](INTERVIEW_QA.md).
