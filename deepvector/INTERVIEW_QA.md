# DeepVector Interview Q&A

> 80+ questions organized by depth with concise, technically precise answers.

## Quick Reference

| # | Topic | Difficulty |
|---|-------|------------|
| 1 | HNSW graph construction | 猸愨瓙猸?|
| 2 | Layer assignment formula | 猸愨瓙 |
| 3 | Search algorithm layer-by-layer | 猸愨瓙 |
| 4 | Role of `ef_search` | 猸?|
| 5 | HNSW time complexity | 猸愨瓙 |
| 6 | HNSW deletes (soft-deletion) | 猸愨瓙 |
| 7 | Why layer 0 uses `2*M` | 猸?|
| 8 | `selectNeighborsSimple` vs heuristic | 猸愨瓙 |
| 9 | AVX2 L2 squared walkthrough | 猸愨瓙猸?|
| 10 | `_mm256_loadu_ps` vs `_mm256_load_ps` | 猸愨瓙 |
| 11 | Why not AVX-512 everywhere? | 猸愨瓙 |
| 12 | What is FMA? | 猸?|
| 13 | Add AVX-512 support | 猸愨瓙猸?|
| 14 | Why not SIMD cosine? | 猸愨瓙 |
| 15 | mmap page fault mechanism | 猸愨瓙猸?|
| 16 | `msync(MS_SYNC)` guarantees | 猸?|
| 17 | `MAP_SHARED` cache coherency | 猸愨瓙 |
| 18 | When to use `read/write` vs mmap | 猸愨瓙 |
| 19 | `grow` remap safety | 猸愨瓙猸?|
| 20 | LSM-Tree write path | 猸愨瓙 |
| 21 | Write amplification & mitigation | 猸愨瓙猸?|
| 22 | Bloom filter in read path | 猸愨瓙 |
| 23 | MemTable structure (skip list) | 猸?|
| 24 | Compaction in MiniKV | 猸愨瓙猸?|
| 25 | PQ high-level explanation | 猸愨瓙 |
| 26 | ADC vs SDC | 猸愨瓙猸?|
| 27 | Why K=256 for PQ? | 猸?|
| 28 | PQ k-means training | 猸愨瓙 |
| 29 | `batchADC` efficiency | 猸愨瓙 |
| 30 | PQ limitations | 猸愨瓙 |
| 31 | PIMPL in DeepVectorServer | 猸愨瓙 |
| 32 | Distance lambda type erasure | 猸愨瓙 |
| 33 | `shared_mutex` vs per-layer locks | 猸愨瓙 |
| 34 | `mutable` keyword usage | 猸?|
| 35 | `const_cast` safety in searchWithFilter | 猸愨瓙 |
| 36 | pybind11 NumPy zero-copy | 猸愨瓙 |
| 37 | GIL release before C++ calls | 猸?|
| 38 | `get_vector` zero-copy NumPy view | 猸愨瓙 |
| 39 | Buffer protocol (PEP 3118) | 猸愨瓙 |
| 40 | LangChain integration structure | 猸?|
| 41 | `select()` vs epoll | 猸愨瓙 |
| 42 | HTTP request parsing | 猸?|
| 43 | Graceful shutdown | 猸?|
| 44 | Connection pooling & keep-alive | 猸愨瓙 |
| 45 | Rate limiting implementation | 猸愨瓙 |
| 46 | Prometheus metrics | 猸愨瓙 |
| 47 | Auth beyond bearer tokens | 猸愨瓙 |
| 48 | Health check endpoints | 猸?|
| 49 | DeepVector vs FAISS | 猸?|
| 50 | DeepVector vs Qdrant | 猸?|
| 51 | DeepVector vs ChromaDB | 猸?|
| 52 | DeepVector vs Milvus | 猸?|
| 53 | When to choose DeepVector | 猸?|
| 54 | Single-node architecture limits | 猸愨瓙 |
| 55 | Scaling to 100M+ vectors | 猸愨瓙猸?|
| 56 | System design: 10K QPS RAG | 猸愨瓙猸?|
| 57 | Testing strategy & HNSW correctness | 猸愨瓙 |
| 58 | mmap persistence testing | 猸愨瓙 |
| 59 | SIMD vs scalar consistency test | 猸愨瓙 |
| 60 | Memory leak detection | 猸愨瓙 |
| 61 | Fuzz testing the HTTP parser | 猸愨瓙 |
| 62 | Concurrent safety testing | 猸愨瓙猸?|
| 63 | CI pipeline | 猸?|
| 64 | Regression handling | 猸?|
| 65 | Server crash mid-write | 猸愨瓙猸?|
| 66 | Debug recall drop from 99% to 80% | 猸愨瓙猸?|
| 67 | Corrupted SSTable handling | 猸愨瓙 |
| 68 | Truncated vector file handling | 猸愨瓙 |
| 69 | Diagnose OOM on HNSW build | 猸愨瓙 |
| 70 | PQ outlier handling | 猸愨瓙 |
| 71 | Integrate with OpenAI embeddings | 猸?|
| 72 | Multi-tenancy | 猸愨瓙 |
| 73 | A/B testing index configurations | 猸愨瓙 |
| 74 | Monitor vector quality drift | 猸愨瓙 |

---

## 1. HNSW Algorithm

### Q1: How does HNSW graph construction work?

HNSW is a multilayer proximity graph. Each node is assigned a random level `l` drawn from an exponential distribution with normalization factor `1/log(M)`. Construction proceeds top-down: starting from the entry point at the topmost layer, we greedily descend one edge per layer until reaching the target node's level. At each layer `lc` from `min(level, max_level_)` down to 0, we run a beam search with `ef_construction` candidates, select the `M` closest neighbors, and establish bidirectional connections. After insertion, we prune neighbors to enforce `M` (or `2M` at layer 0) connections per node.

### Q2: What is the layer assignment formula and why?

Level `l = floor(-ln(uniform(0,1)) * mL)`, where `mL = 1/ln(M)`. This produces an exponentially decaying distribution 鈥?most nodes live only at layer 0, and the probability of reaching level `l` is `(1/M)^l`. This creates the "highway" property: the top layer has few long-range edges enabling logarithmic search complexity. With M=16, ~6.25% of nodes reach level 1, ~0.4% reach level 2.

### Q3: Explain the search algorithm layer by layer.

1. Start at the entry point (topmost layer).
2. For each layer from `max_level_` down to 1: greedily descend with `ef=1` 鈥?evaluate distance to all neighbors of the current node, move to the closest, repeat until no improvement. This finds the local minimum at that layer, which becomes the entry point for the next layer down.
3. At layer 0 (the dense layer): run `searchLayer` with `ef_search` candidates. This is a beam search 鈥?maintain a candidate priority queue (min-heap by distance) and a result set (max-heap, at most `ef` elements). For each candidate, explore all unvisited neighbors; if a neighbor's distance is better than the worst result, add it to both queues. Terminate when the candidate queue's nearest element is farther than the worst result.
4. Sort the `ef_search` candidates by distance, take top-k.

### Q4: What is the role of `ef_search` and how do you tune it?

`ef_search` controls the search width 鈥?the number of candidates explored at layer 0. Higher values increase recall at the cost of latency. The default is `ef_construction=200` set at construction time. For queries:
- `ef_search=16` 鈫?90-95% recall, very fast
- `ef_search=50` 鈫?95-98% recall
- `ef_search=200` 鈫?99%+ recall

Tuning strategy: start with `ef_search = k 脳 4`, increase until recall meets SLA. The relationship is sublinear 鈥?doubling `ef_search` adds ~30% latency for ~2% additional recall.

### Q5: What is the time complexity of HNSW search and insert?

Search: O(log N 脳 M) on average 鈥?`log N` layers with `M` neighbor evaluations per step during greedy descent, plus `ef_search 脳 M` evaluations at layer 0. In practice, this is closer to O(log N) for modest `ef_search`.

Insert: O(log N 脳 M 脳 ef_construction) 鈥?search for entry points at each of `log N` layers, each using `ef_construction` beam width. The dominant cost is the layer 0 search which explores `ef_construction 脳 M` candidates.

### Q6: How does HNSW handle deletes?

Via soft-deletion: the node's `deleted` flag is set to true, and `element_count_` is decremented. The node remains in the graph's neighbor lists to maintain connectivity for other nodes. Valid results filter out deleted nodes during the final candidate collection. This avoids expensive graph repair at the cost of some "zombie" edges that may be traversed during search.

### Q7: Why does layer 0 use `M_max0_ = 2*M` while upper layers use M?

Layer 0 is the densest layer (contains all nodes) and carries the bulk of search traffic. Doubling the connections (32 vs 16 for M=16) improves recall at the cost of ~2脳 memory for neighbor lists. Upper layers are sparse (only ~1/M of nodes per additional layer), so the memory overhead of 2脳 connections would be negligible but also unnecessary 鈥?the greedy descent at upper layers only needs 1 connection to navigate efficiently.

### Q8: What's the difference between `selectNeighborsSimple` and heuristic neighbor selection?

`selectNeighborsSimple` greedily picks the M closest candidates. The heuristic variant (not yet implemented in this codebase) considers whether a candidate is closer to the query than its existing neighbors 鈥?it prunes candidates that would create redundant edges. Heuristic selection produces graphs with better navigability and recall but is O(M虏) vs O(M log M) for the simple version. The current codebase uses simple selection for speed.

---

## 2. SIMD Optimization

### Q9: Walk through the AVX2 L2 squared distance implementation.

```cpp
__m256 sum = _mm256_setzero_ps();           // 8-wide zero accumulator
for (; i + 8 <= dim; i += 8) {
    __m256 va = _mm256_loadu_ps(a + i);     // unaligned load 8 floats
    __m256 vb = _mm256_loadu_ps(b + i);
    __m256 diff = _mm256_sub_ps(va, vb);
    sum = _mm256_fmadd_ps(diff, diff, sum);  // fused multiply-add
}
// Horizontal sum: store 8 lanes, scalar add
float result[8];
_mm256_storeu_ps(result, sum);
float total = result[0]+result[1]+result[2]+result[3]
            + result[4]+result[5]+result[6]+result[7];
// Tail: scalar loop for remaining dim % 8 elements
```

Each iteration processes 8 dimensions in ~4 cycles (sub + fmadd can pipeline). For 768-dim, that's 96 iterations at 96脳4 鈮?384 cycles vs scalar's 768脳5 鈮?3840 cycles 鈥?roughly 10脳 speedup on the SIMD portion.

### Q10: Why use `_mm256_loadu_ps` instead of `_mm256_load_ps`?

`_mm256_load_ps` requires 32-byte aligned addresses. Vector data from mmap'd buffers, NumPy arrays, and general heap allocations is not guaranteed to be 32-byte aligned. Using `loadu` avoids alignment constraints at the cost of 1 extra cycle on modern CPUs (Haswell and later have unified load ports). For strided access patterns in HNSW graph traversal, alignment is unpredictable, so `loadu` is the safe default.

### Q11: Why not AVX-512 everywhere?

1. **Market penetration**: AVX-512 is available on Intel Skylake-X/SP+, Ice Lake+ client, and AMD Zen 4+. Many cloud instances (especially cost-optimized ones) still run on AVX2-only CPUs.
2. **Frequency throttling**: AVX-512 heavy instructions can cause clock frequency reductions (licensing-based throttling on Intel), sometimes making scalar code faster for mixed workloads.
3. **Binary portability**: An AVX-512 binary crashes on AVX2-only hardware. Multi-version dispatch (function multi-versioning or runtime dispatch) adds complexity.
4. **Marginal gain for 768-dim vectors**: AVX2 processes 8 floats, AVX-512 processes 16. The 2脳 theoretical speedup diminishes after amortizing the horizontal sum, tail handling, and the fact that HNSW graph traversal (not distance computation) is often the bottleneck.

### Q12: What is FMA and why does it matter here?

Fused Multiply-Add (`_mm256_fmadd_ps`) computes `a*b + c` with a single rounding step instead of two (multiply then add). For distance computation: `diff*diff + sum` becomes one instruction instead of a multiply followed by an add. Benefits: (1) fewer uops 鈫?higher throughput, (2) reduced rounding error since intermediate results aren't rounded, (3) lower latency (4 cycles vs 3+3 for separate mul+add).

### Q13: How would you add AVX-512 support?

```cpp
#ifdef __AVX512F__
inline float l2_squared_avx512(const float* a, const float* b, size_t dim) {
    __m512 sum = _mm512_setzero_ps();
    size_t i = 0;
    for (; i + 16 <= dim; i += 16) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 diff = _mm512_sub_ps(va, vb);
        sum = _mm512_fmadd_ps(diff, diff, sum);
    }
    return _mm512_reduce_add_ps(sum);  // Built-in horizontal sum on AVX-512
    // + scalar tail loop
}
```

Then add a dispatch layer that checks `__builtin_cpu_supports("avx512f")` at runtime or uses function multi-versioning with `__attribute__((target("avx512f")))`.

### Q14: Why isn't cosine distance SIMD-accelerated?

Cosine = `1 - dot(a,b) / sqrt(|a|*|b|)`. While the dot product can be SIMD-accelerated (identical to inner product), computing the norms `|a|` and `|b|` requires either: (a) pre-stored norms (extra memory), (b) computing norms at query time (O(dim) per query, acceptable), or (c) computing norms for each candidate during search (O(dim) per candidate, unacceptable). The final division `1 - dot/sqrt(na*nb)` is inherently scalar. The codebase opts for scalar cosine to keep the implementation simple 鈥?the gain from SIMD acceleration of the dot product alone is marginal when amortized over the square root and division.

---

## 3. mmap vs Traditional I/O

### Q15: Explain the mmap page fault mechanism for vector access.

When `VectorStore::get(id)` returns a pointer into the mmap'd region, no I/O has occurred yet. When the CPU first reads that address, a page fault triggers the kernel to:
1. Check if the page is already in the page cache (yes 鈫?map PTE, return).
2. If not, read the page from disk into a page cache frame.
3. Update the process page table entry to point to the cached frame.
4. Return to userspace, re-execute the faulting instruction.

Subsequent reads hit the page cache. Writes to `MAP_SHARED` pages dirty the page cache entry; the kernel writes back asynchronously (configurable via `vm.dirty_ratio`).

### Q16: What does `msync(MS_SYNC)` guarantee?

`msync(MS_SYNC)` blocks until all dirty pages in the specified range are written to the backing store (disk). This provides durability equivalent to `fsync` on a `write()` 鈥?after `msync` returns, the data is on disk. `MS_ASYNC` schedules the write but returns immediately (no durability guarantee). `MS_INVALIDATE` invalidates cached copies so subsequent reads go to disk (rarely useful for vector databases).

### Q17: What are the OS cache coherency implications of MAP_SHARED?

`MAP_SHARED` means:
- **Inter-process**: Writes are visible to all processes that mapped the same file. The kernel's unified page cache ensures this 鈥?there's only one copy of each page in memory, shared across all mappers.
- **mmap vs read/write**: A `read()` after an `mmap` write always sees the latest data because both go through the same page cache. The converse is also true.
- **Cache-line level**: On x86, cache coherency is handled by the MESI protocol at the hardware level. The kernel page cache provides the software-level guarantee that all mappings see the same physical page.

### Q18: When would you use `read/write` instead of mmap?

1. **Streaming access**: Sequential read-once workloads benefit from `read()`'s readahead without the overhead of setting up and tearing down VMAs.
2. **Small files**: mmap overhead (page tables, TLB pressure) outweighs the benefit for files under ~64KB.
3. **Write-heavy with frequent fsync**: mmap's asynchronous writeback makes it hard to know exactly when data hits disk. Explicit `write+fsync` gives precise control.
4. **Memory-constrained systems**: mmap's page eviction is opaque 鈥?the kernel may evict pages you need soon. With `read`, you control buffer lifetime explicitly.
5. **Growing files beyond address space**: On 32-bit systems or very large datasets, you may not have enough virtual address space.

### Q19: How does the `grow` operation handle remapping safely?

The `grow` method in VectorStore:
1. Calls `msync` to flush dirty data.
2. Calls `munmap` to unmap the old region (all pointers into it are now invalid).
3. Calls `ftruncate` to extend the file.
4. Calls `mmap` at the new size.

This is safe because there's no concurrent access during grow (single-threaded server). A production system would need either: (a) stop-the-world during grow, (b) double-buffering (map a new region, copy, swap pointers atomically, unmap old), or (c) map in segments (multiple mmap calls covering the file in chunks).

---

## 4. LSM-Tree Design (MiniKV)

### Q20: Describe the write path in an LSM-Tree.

1. Write arrives 鈫?append to WAL (Write-Ahead Log) 鈫?`fsync` if `wal_sync=true`.
2. Insert into MemTable (in-memory sorted structure, typically a skip list).
3. When MemTable reaches size threshold 鈫?convert to immutable MemTable, flush to disk as SSTable (Sorted String Table).
4. New MemTable starts accepting writes.

Read path: Check MemTable 鈫?check immutable MemTables 鈫?check SSTables from newest to oldest (Level 0 may overlap; Level 1+ are non-overlapping). Bloom filters skip SSTables that don't contain the key.

### Q21: What is write amplification and how does MiniKV mitigate it?

Write amplification = (total bytes written to disk) / (bytes of user writes). In LSM-trees, compaction reads multiple SSTables, merges them, and writes new ones 鈥?each byte written by the user may be rewritten several times.

Mitigations in MiniKV:
- Leveled compaction: SSTables are organized in levels; Level 0 is ~10MB, Level L+1 is 10脳 Level L. Data is moved through fewer levels.
- Bloom filters: False-positive rate of ~1% means 99% of point lookups avoid unnecessary SSTable reads.
- Configurable `wal_sync`: Trading durability for throughput when `wal_sync=false` reduces synchronous fsync overhead.

### Q22: How does the Bloom filter work in MiniKV's read path?

A Bloom filter is a probabilistic data structure with k hash functions mapping keys to m bits. On put: set bits `h1(key)...hk(key)`. On get: check all k bits 鈥?if any is 0, the key is definitely absent (no false negatives). If all are 1, the key might be present (configurable false positive rate).

Read path: `get(key)` 鈫?for each SSTable (from newest) 鈫?check Bloom filter 鈫?if "absent", skip 鈫?if "present", do a binary search within the SSTable's index block 鈫?read data block 鈫?scan for key.

### Q23: What is the MemTable structure in MiniKV?

MiniKV uses a concurrent skip list as its MemTable implementation. A skip list is a probabilistic balanced tree where each node has a random height (similar to HNSW's layer assignment). Insert/search complexity is O(log N) expected. The skip list is lock-free for reads and uses per-node locks for writes, enabling concurrent read-heavy workloads without blocking.

### Q24: Explain compaction in MiniKV.

When Level 0 reaches its size threshold (e.g., 4 files), MiniKV triggers compaction:
1. Select all Level 0 files (they may overlap in key range).
2. Find overlapping files in Level 1.
3. Merge-sort all selected files (iterator over each file, k-way merge).
4. Write new SSTables for Level 1 (non-overlapping).
5. Delete old files.

This is worst-case write amplification: Level 0 files may overlap with many Level 1 files, potentially rewriting 10脳 the data. Tiered compaction (RocksDB universal compaction) trades worse read amplification for better write amplification.

---

## 5. Product Quantization

### Q25: Explain Product Quantization at a high level.

PQ decomposes the original D-dimensional vector space into M subspaces of dimension `dsub = D/M`. Each subspace has its own codebook of K centroids (trained via k-means). A vector is encoded as M bytes, where byte `i` indexes the closest centroid in subspace `i`'s codebook. The total memory for a vector drops from `D 脳 4` bytes (float32) to `M` bytes (uint8 codes). For DeepVector default: 768 脳 4 = 3072 bytes 鈫?96 bytes (32脳 compression).

### Q26: What is the difference between ADC and SDC?

**ADC (Asymmetric Distance Computation)**: The query vector is not quantized 鈥?it's compared directly against the codebooks. For each subspace, compute the distance from the query's subvector to all K centroids 鈫?distance table of size M 脳 K. Then for each candidate code, sum up `dist_table[m][code[m]]`. O(K 脳 dsub 脳 M) for table construction, O(M) per candidate.

**SDC (Symmetric Distance Computation)**: Both query and candidate are quantized. Precompute distance between every pair of centroids within each subspace 鈫?M lookup tables of size K 脳 K. Distance between two codes is the sum of `table[m][code_a[m]][code_b[m]]`. O(M 脳 K虏 脳 dsub) for precomputation (once), O(M) per candidate.

ADC has higher accuracy (query is not quantized) and lower precomputation cost. SDC is better when you have a fixed query set or when comparing code-to-code.

### Q27: Why is K=256 the typical choice for PQ?

K=256 means each code is 1 byte (uint8_t), which is: (a) the smallest practical integer type, (b) trivially aligned in memory, (c) maps well to CPU byte-addressing, (d) provides enough granularity 鈥?beyond 256 centroids per subspace, the marginal recall gain diminishes because the quantization error is dominated by the subspace decomposition, not the codebook resolution.

### Q28: How does DeepVector train PQ using k-means per subspace?

For each subspace `m` in `[0, M)`:
1. Extract subspace `m` from all training vectors 鈫?subVectors of size `n 脳 dsub`.
2. Initialize K centroids by random sampling (fixed seed 42 for reproducibility).
3. Lloyd iteration (max 25): assign each subvector to nearest centroid, recompute centroids as mean of assignments.
4. Store centroids in `centroids_` array: `M 脳 K 脳 dsub` floats.

No k-means++ initialization (random only) 鈥?this is a deliberate simplicity trade-off. k-means++ would reduce iterations needed for convergence but adds O(n 脳 K 脳 dsub) time to initialization. For typical training sets (10K+ vectors), random initialization with more iterations converges to comparable quality.

### Q29: How does batchADC work and why is it efficient?

`batchADC(query, codes, n, distances)`:
1. Compute distance table once: O(M 脳 K 脳 dsub).
2. For each of n codes: sum M table lookups: O(n 脳 M).

The distance table construction is amortized over n lookups, making per-vector cost just O(M) table lookups. This is crucial for search 鈥?the HNSW search function calls `query_dist_` once per candidate, and if it's the PQ path, each call does the table lookup. For ef_search=50 candidates, that's 50 脳 96 = 4800 table lookups vs re-computing full L2 distance (50 脳 768 = 38400 operations).

### Q30: What are the limitations of PQ?

1. **Subspace independence assumption**: Assumes dimensions within each subspace are correlated but cross-subspace dimensions are independent. Violates reality for embeddings where all dimensions interact.
2. **Training data drift**: Codebooks trained on one distribution work poorly on another. RAG workloads with continually added documents may need periodic retraining.
3. **K-means convergence**: Random initialization may settle on poor local minima, especially with high `dsub`. k-means++ or PCA-based initialization can help.
4. **Memory-quality trade-off**: For very high recall demands (>99%), PQ's quantization error may be unacceptable. Additive quantizers (LSQ, RQ) offer better quality at higher encoding cost.

---

## 6. C++ Design Patterns

### Q31: Why does DeepVectorServer use PIMPL?

The `DeepVectorServer::Impl` class hides POSIX socket internals (`select`, `fd_set`, `sockaddr_in`, `fcntl`) from the public header `server.h`. This prevents: (a) transitive `#include <sys/socket.h>` pollution for consumers, (b) ABI breakage if the networking implementation changes (e.g., from `select` to epoll), (c) exposure of platform-specific types that harm portability. The server header only depends on standard library types.

### Q32: How are the distance lambdas in Collection using type erasure?

`HNSWIndex` stores `std::function<float(uint64_t, uint64_t)>` and `std::function<float(uint64_t, const float*)>`. In `Collection`'s constructor, lambdas capture `this` and call quantized or raw distance functions based on runtime state:

```cpp
index_->setPairwiseDistance([this](uint64_t a, uint64_t b) -> float {
    if (pq_ && pq_->isTrained()) { ... return pq_->symmetricDistance(...); }
    if (sq_ && sq_->isTrained()) { ... return sq_->l2SquaredDistance(...); }
    return index::compute_distance(va, vb, config_.dim, config_.metric);
});
```

The `std::function` erases the lambda type; the HNSW index calls these opaque callbacks without knowing whether PQ, SQ, or raw distance is in use. The cost is one virtual call per distance evaluation 鈥?acceptable because the computation inside the call is O(dim).

### Q33: Why `shared_mutex` instead of per-layer locks?

`std::shared_mutex` provides the classic readers-writer pattern: many concurrent `search()` calls (shared lock) and exclusive `insert()` calls (unique lock). Per-layer locking (one mutex per HNSW level) could increase read concurrency (readers on different layers don't block each other) but adds: (a) lock ordering complexity to avoid deadlocks during insert which traverses all layers, (b) more memory per mutex, (c) diminishing returns since layer 0 dominates search time (~99% of distance evaluations). The single `shared_mutex` is simpler and proven correct.

### Q34: What is the `mutable` keyword doing for `ef_search_` and `rng_`?

`mutable` allows modification of member variables in `const` member functions. `ef_search_` is mutable because `search()` is `const` (it doesn't change the graph structure) but `searchWithFilter` needs to temporarily change `ef_search_` to widen the search. `rng_` is mutable because `randomLevel()` needs to mutate the RNG state even when called during a conceptually read-only code path.

### Q35: Explain the `const_cast` in `searchWithFilter` and its safety.

```cpp
const_cast<index::HNSWIndex*>(index_.get())->setEfSearch(ef);
// ... search ...
const_cast<index::HNSWIndex*>(index_.get())->setEfSearch(prev_ef);
```

This is safe because: (a) `ef_search_` is `mutable`, so the const_cast is technically unnecessary but expresses intent, (b) the entire operation is within a single function 鈥?`ef_search_` is restored before returning, (c) `searchWithFilter` takes exclusive access during the widened search. The alternative (making `setEfSearch` const and marking `ef_search_` mutable) would be cleaner.

---

## 7. Python Bindings (pybind11)

### Q36: How does pybind11 bridge NumPy arrays to C++ pointers?

The binding uses `py::array_t<float, py::array::c_style | py::array::forcecast>`:
- `c_style`: ensures row-major contiguous layout (or throws).
- `forcecast`: automatically converts compatible dtypes (e.g., float64 鈫?float32).
- `buf.request()`: acquires the buffer protocol 鈫?`py::buffer_info` with `.ptr` (raw data pointer) and `.ndim` (dimensions).
- The C++ code reads directly from the NumPy buffer's memory (no copy).

### Q37: Why release the GIL before calling C++ code?

```cpp
py::gil_scoped_release release;
return self.add(data);
```

`Collection::add` performs HNSW graph insertion, which can be computationally heavy (thousands of distance computations). Holding the GIL during this would block all other Python threads, including those not using DeepVector. Releasing the GIL allows true multi-threaded Python programs to run DeepVector operations concurrently with other work. The GIL is re-acquired automatically when `release` goes out of scope.

### Q38: How does `get_vector` return a zero-copy NumPy view?

```cpp
.def("get_vector", [](Collection& self, uint64_t id) -> py::object {
    const float* v = self.getVector(id);
    if (!v) return py::none();
    return py::array_t<float>(self.dim(), v);  // no copy
})
```

`py::array_t<float>(dim, ptr)` creates a NumPy array that *views* the existing memory without copying. The array's data pointer points directly into the mmap'd region. Warning: if the VectorStore grows (remmap occurs), the pointer is invalidated. The caller must not hold these arrays across inserts that could trigger a grow.

### Q39: What is the buffer protocol and why does pybind11 use it?

The Python buffer protocol (PEP 3118) is a C-level interface allowing objects to expose their raw memory to other objects without copying. NumPy arrays, `bytes`, `bytearray`, `memoryview`, and `array.array` all implement it. pybind11 uses it so DeepVector bindings work with any buffer-like object, not just NumPy arrays 鈥?at the cost of requiring `c_style` and `forcecast` for type safety.

### Q40: How is the LangChain integration structured?

`DeepVectorVectorStore` extends LangChain's `VectorStore` base class. It wraps a `Collection` instance and an embedding function. `add_texts` computes embeddings via the provided embedding model, creates `DocumentMeta` objects with text content and optional metadata, and inserts via `add_with_meta`. `similarity_search` embeds the query, calls `search(k)`, retrieves metadata for each result, and returns LangChain `Document` objects. The integration is lazy-imported 鈥?if LangChain is not installed, it degrades gracefully.

---

## 8. HTTP Server Design

### Q41: Why `select()` instead of epoll?

`select()` was chosen for simplicity: (a) it's universal POSIX 鈥?works on Linux, macOS, BSD without `#ifdef`, (b) the current single-threaded model is I/O-bound at read/write, not at event dispatch, (c) for <1000 connections, `select`'s O(n) fd_set scanning is negligible. The `select` implementation is 80 lines vs epoll's ~200 lines when accounting for edge-triggered state management, EAGAIN handling, and event batching.

### Q42: How does the server parse HTTP requests?

The server uses hand-rolled HTTP/1.1 parsing (no external HTTP library):
- `methodFromRequest`: find first space 鈫?extract method
- `pathFromRequest`: find second space 鈫?extract path
- `bodyFromRequest`: find `\r\n\r\n` 鈫?everything after is body
- `checkAuth`: substring search for `Authorization: Bearer <key>`

This is intentionally minimalist 鈥?a full HTTP parser would handle chunked transfer encoding, header continuation lines, trailing headers, and request pipelining. For an internal-facing vector database where the client is typically a Python library or microservice, basic HTTP/1.1 with JSON bodies is sufficient.

### Q43: How is graceful shutdown implemented?

1. `SIGINT`/`SIGTERM` handler sets `running = false` (atomic).
2. Main thread's `while (running)` loop exits.
3. `server.stop()` is called, which sets `running_ = false` and joins the server thread.
4. The event loop exits `while (running_)`, closes all client fds, closes the listen fd.
5. `Collection` destructor calls `store_->flush()` (msync).
6. Process exits.

### Q44: Where would connection pooling and keep-alive fit?

The server already sends `Connection: keep-alive` in responses, but the current implementation reads once, responds once, and closes. To add keep-alive: (a) track per-fd read buffers with partial request accumulation, (b) after writing the response, reset the buffer state and return to reading without closing, (c) add idle timeout per connection (e.g., 30s), (d) track max concurrent requests per connection to prevent resource exhaustion.

---

## 9. Production Concerns

### Q45: How would you add rate limiting?

Token bucket algorithm per API key:
1. Maintain a `map<api_key, TokenBucket>` where each bucket has `tokens` (current count) and `last_refill` timestamp.
2. Config: `rate` (tokens/second), `burst` (max tokens).
3. On each request: `tokens = min(burst, tokens + rate * (now - last_refill))`. If `tokens >= 1`, decrement and allow. Else, return 429.
4. Cleanup: sweep expired entries periodically to avoid memory leak.

### Q46: How would you add Prometheus metrics?

Add a `/metrics` endpoint that returns Prometheus text format:
```
lumendb_requests_total 12345
lumendb_search_duration_seconds{quantile="0.5"} 0.0002
lumendb_search_duration_seconds{quantile="0.99"} 0.001
lumendb_vectors_count 1000000
lumendb_memory_bytes 3221225472
```

Use a histogram for search latency 鈥?`std::chrono::high_resolution_clock` around the search call, record in buckets (100碌s, 500碌s, 1ms, 5ms, 10ms). The `ServerStats` counters already capture basic counters.

### Q47: How would you implement authentication beyond bearer tokens?

1. **HMAC-based**: Client signs request body + timestamp with shared secret. Server verifies. Prevents replay attacks if timestamp is within a window (e.g., 5 min).
2. **mTLS**: Client presents certificate. Server validates against CA. No per-request authentication overhead.
3. **JWT with OAuth2**: For multi-tenant deployments. Each collection maps to a tenant; JWT claims carry `tenant_id`.
4. **API key hashing**: Store `SHA256(key)` not plaintext keys in the server config.

### Q48: What health check endpoints should a production deployment have?

- `/health`: Liveness 鈥?returns 200 if the process is running. No dependency checks. Used by orchestrator to restart crashed processes.
- `/ready`: Readiness 鈥?returns 200 only if the server can accept requests. Checks: mmap is accessible, MiniKV is open, thread pool is alive.
- `/metrics`: Prometheus metrics endpoint (see Q46).
- `/debug/pprof`: CPU and heap profiling endpoints (if built with gperftools or similar).

---

## 10. Comparison to Alternatives

### Q49: How does DeepVector compare to FAISS?

| Aspect | DeepVector | FAISS |
|--------|---------|-------|
| Language | C++17 | C++14/17 |
| Server mode | Built-in HTTP server | External wrapper needed |
| Index types | HNSW only | IVF, HNSW, PQ, OPQ, IMI, NSG, etc. |
| GPU support | None | Full CUDA support |
| Distance metrics | L2, IP, Cosine | L2, IP, Cosine, L1, Lp, etc. |
| Filters | AST-based with metadata | IDSelector only (no metadata) |
| Insertion | Incremental (zero training) | IVF requires training; HNSW is incremental |
| Python bindings | pybind11 + LangChain | SWIG + official Python package |
| Compression | PQ + SQ | PQ, OPQ, SQ, IVFPQ |

FAISS wins on: index variety, GPU acceleration, and research-backed optimizations (OPQ, polysemous codes). DeepVector wins on: built-in HTTP server, metadata filtering, LangChain integration, and deployment simplicity (single binary).

### Q50: How does DeepVector compare to Qdrant?

| Aspect | DeepVector | Qdrant |
|--------|---------|--------|
| Language | C++ | Rust |
| Storage engine | mmap + MiniKV LSM | RocksDB (via rocksdb crate) |
| Protocol | HTTP/1.1 JSON | gRPC + REST |
| Distributed | No | Yes (Raft consensus) |
| Payload filtering | AST-based | Full JSON payload indexing |
| Quantization | PQ + SQ | Scalar + Product + Binary |
| Auth | Bearer token | API key |
| Deployment | Single binary (~2MB) | Docker container (~50MB+) |

Qdrant wins on: distributed operation, rich payload indexing (keyword, integer, geo), gRPC performance, and production maturity. DeepVector wins on: minimal resource footprint, zero-copy mmap performance, C++ embeddability, and codebase simplicity (~3K LOC vs ~100K LOC).

### Q51: How does DeepVector compare to ChromaDB?

| Aspect | DeepVector | ChromaDB |
|--------|---------|----------|
| Language | C++ | Python (with Rust cores) |
| Embedding | Bring your own | Built-in embedding functions |
| Storage | mmap + MiniKV | SQLite3 + DuckDB |
| Index | HNSW | HNSW (hnswlib) |
| Maturity | Prototype | Production |
| LangChain | Supported | First-party |
| Deployment | Single binary or pip install | pip install + ClickHouse/SQLite |

ChromaDB wins on: developer experience (built-in embeddings, rich Python API, extensive documentation), production maturity, and community. DeepVector wins on: raw performance (10-50脳 faster search due to C++ vs Python), memory efficiency, and embeddability in non-Python environments.

### Q52: How does DeepVector compare to Milvus?

| Aspect | DeepVector | Milvus |
|--------|---------|--------|
| Architecture | Embedded library | Distributed (proxy + data/index/query nodes) |
| Index types | HNSW | 11+ index types (IVF, HNSW, DiskANN, etc.) |
| Consistency | Immediate | Tunable (strong/bounded/eventual) |
| Metadata | KV store | Scalar field indexing |
| Deployment | Single binary | Kubernetes operator / Docker Compose |
| Target scale | 1M-10M vectors | 1B+ vectors |
| Complexity | ~3K LOC | ~500K+ LOC |

Milvus is a distributed, cloud-native system for billion-scale vector search with tunable consistency and multi-tenancy. DeepVector is an embedded library for single-node RAG workloads. They target completely different scales. DeepVector could serve as Milvus's local index cache (like Milvus Lite), but is not a replacement.

### Q53: When should someone choose DeepVector over alternatives?

Choose DeepVector when:
1. You need a **single-binary deployment** with no external dependencies (no JVM, no Python runtime, no Docker required).
2. You're building a **RAG pipeline** with LangChain and want sub-millisecond vector search embedded in your process.
3. You have **1K-10M vectors** and care about resource efficiency (DeepVector runs comfortably in <100MB RAM with PQ).
4. You need **metadata filtering** integrated with ANN search (pre-filter or post-filter depending on selectivity).
5. You want to **embed directly into C++** applications (game engines, edge devices, on-premise appliances).

Don't choose DeepVector when:
1. You need **billion-scale** search 鈫?use Milvus or Qdrant cluster.
2. You need **GPU acceleration** 鈫?use FAISS.
3. You need **distributed high availability** 鈫?use Qdrant with Raft.
4. You need **managed cloud service** 鈫?use Pinecone, Zilliz Cloud, or Qdrant Cloud.

### Q54: What's the architecture limitation of a single-node vector database?

1. **Memory ceiling**: If vectors + index don't fit in RAM, disk-based search becomes 100-1000脳 slower (DiskANN mitigates this with SSD-aware graph layout, but DeepVector doesn't implement it).
2. **No replication**: Disk failure = data loss. Mitigation: periodic `msync` + external backup.
3. **Throughput ceiling**: One server can handle ~10K QPS (HNSW search on 1M vectors). Beyond that, you need sharding.
4. **No online schema changes**: Changing dimension or distance metric requires full rebuild.

### Q55: How would you scale DeepVector to 100M+ vectors?

1. **Sharding by ID range**: Partition vectors across N nodes. Route queries to all shards, merge top-k results. O(N) network overhead per query.
2. **Sharding by centroid (IVF)**: Coarse quantization splits the space into N clusters. Route query to the nearest C clusters (C << N). Reduces fan-out.
3. **Hierarchical**: A root node routes to the top-K shards based on coarse centroids; shard nodes do HNSW search locally.
4. **Replication**: Each shard has 2-3 replicas for HA and read throughput.
5. **Consistent hashing**: Use hash ring for elastic scaling 鈥?add/remove nodes without full data reshuffle.

---

## Bonus: System Design Scenario

### Q56: Design a RAG system using DeepVector that handles 10K QPS at P99 < 100ms.

**Architecture**:
- **Ingest pipeline**: Kafka 鈫?embedding service (GPU) 鈫?DeepVector insert
- **Query pipeline**: API gateway 鈫?query embedding 鈫?DeepVector search 鈫?LLM
- **DeepVector config**: M=64, ef_search=100, PQ enabled with M=96, sharded across 4 nodes by ID range
- **Caching**: Redis cache for frequent queries (embedding 鈫?top-k results), TTL=5min
- **Load balancing**: Round-robin across shards for inserts; fan-out to all shards for search, merge with priority queue

**Bottleneck analysis**:
- Embedding generation: ~5ms on GPU 鈫?parallelizable
- DeepVector search: ~0.5ms per shard 鈫?0.5ms total (parallel fan-out)
- Network: <1ms intra-DC
- Merge top-k: ~0.1ms with heap
- LLM generation: ~50ms (dominant)
- **Total**: ~56ms 鈫?within 100ms budget

**Capacity planning**: 4 shards 脳 2 replicas = 8 nodes. Each node handles 1.25K QPS for search + 2.5K QPS for insert. HNSW with ef_search=100 at 2.5M vectors per node 鈫?well within the <1ms P99 envelope.

---

## 11. C++ Optimization & Trade-offs

### Q57: Why `std::vector` of `HNSWNode` (indexed by ID) instead of a hash map?

The `nodes_` vector uses the ID as index: `nodes_[id]`. This gives O(1) access vs O(1) avg for hash maps, but with better cache locality (contiguous memory). The trade-off: memory is sparse 鈥?deletions leave holes (zombie nodes), and the vector resizes to `max(id) + 1`. For dense ID assignment (monotonically increasing), this works well. For sparse IDs (e.g., distributed generation), it wastes memory. Production systems might use `absl::flat_hash_map` or a two-level index (coarse page + fine offset).

### Q58: Why are the scratch buffers `mutable`?

`pq_scratch_a_`, `pq_scratch_b_`, `sq_scratch_a_`, `sq_scratch_b_`, and `pq_dist_table_` are `mutable` because they're used in the distance callbacks which are invoked from `search()` (a `const` method). These buffers are thread-local in effect (each thread has its own scratch space via thread-local storage, or in this single-threaded server context, there's only one caller at a time). Without `mutable`, every distance computation would allocate new vectors 鈥?unacceptable overhead.

### Q59: What happens if `pairwise_dist_` or `query_dist_` is not set?

`insert()` returns immediately without modifying state (`if (!pairwise_dist_ || !query_dist_) return;`). `search()` returns an empty vector. The HNSWIndex itself is agnostic to how vectors are stored 鈥?it only cares about IDs and distances. The callbacks must be set before any operation. This decoupling allows testing with side arrays (as in `test_hnsw.cpp`) or with production backends (mmap, PQ, SQ).

### Q60: Why does `insert()` swap `query_dist_` with a lambda?

During insertion, the algorithm needs to find neighbors of the new node within existing nodes. The `searchLayer` function uses `query_dist_` to compute distances. But `query_dist_` takes `(uint64_t id, const float* query)` 鈥?there's no "query" during insertion. The trick: temporarily replace `query_dist_` with a lambda that captures the new node's ID and calls `pairwise_dist_(id, other)`:

```cpp
auto insertQuery = [this, id](uint64_t other, const float*) -> float {
    return pairwise_dist_(id, other);
};
auto savedQuery = query_dist_;
query_dist_ = insertQuery;
// ... insert logic ...
query_dist_ = savedQuery;
```

The `const float*` parameter is ignored (passed as `nullptr` from the insert path). This reuses the same `searchLayer` code path without duplicating the beam search logic.

---

## 12. Testing & Quality

### Q61: What's your testing strategy? How do you test HNSW correctness?

Test pyramid approach:
- **Unit tests**: Distance functions (L2, inner product, cosine) with known inputs/expected outputs. Filter evaluation with all operators and tree combinations. PQ and SQ encode/decode roundtrip with error bounds.
- **Integration tests**: HNSW 鈥?insert N vectors, verify self-search returns self as first result with distance ~0. Collection 鈥?insert vectors with metadata, search, verify metadata integrity.
- **Property-based**: All returned distances are non-negative. Search results are monotonic (results[0].distance <= results[1].distance). searchWithFilter only returns results matching the filter.
- **Recall tests**: Compare HNSW results against brute-force exact KNN on small datasets (<1000 vectors). The `RecallTest` in `test_hnsw.cpp` verifies at least 50% recall@10 (deliberately low bar for small datasets where HNSW parameters aren't fully tuned). For production-grade validation, compare against an exhaustive scan on up to 10K vectors.
- CI runs all tests on every push.

### Q62: How do you test that mmap persists correctly after restart?

Collection roundtrip test strategy:
1. Create Collection, insert N vectors with known values.
2. Call `coll.save()` (which calls `store_->flush()` 鈫?`msync(MS_SYNC)`).
3. Destroy the Collection object.
4. Create a new Collection from the same data directory path.
5. Verify `coll.size()` matches the original count.
6. Verify `coll.getVector(id)` returns the exact same float32 values for randomly sampled IDs.
7. Verify metadata roundtrip: `coll.getMeta(id)` returns correct `DocumentMeta`.

VectorStore also has a dedicated roundtrip test: create 鈫?append 鈫?flush 鈫?`VectorStore::load()` from the same path 鈫?verify count, dim, and data integrity. The `test_collection.cpp` already covers the basic flow.

### Q63: How do you verify SIMD correctness vs scalar?

`test_distance.cpp` includes `SIMDvsScalarConsistency`: generates 100 random vectors of dim=128, computes both SIMD and scalar distances, verifies relative error < 1e-4. This catches off-by-one errors in the SIMD loop, incorrect horizontal sum ordering, and missing tail handling. The test uses `l2_distance` (which returns sqrt of l2_squared internally) and `l2_squared` directly, checking that `l2 * l2 鈮?l2sq`.

### Q64: How do you catch memory leaks?

- **ASAN (AddressSanitizer)**: Included in CI via `vendor/MiniKV/cmake/Sanitizers.cmake`. Detects heap buffer overflows, use-after-free, and memory leaks.
- **Destructor hygiene**: `HNSWIndex` destructor implicitly deletes all `HNSWNode` objects via `std::vector`. `Collection` pimpl uses `std::unique_ptr` for `index_`, `store_`, `docs_`, `pq_`, `sq_`.
- **mmap cleanup**: `VectorStore` destructor calls `munmap` and `close(fd_)`.
- Manual leak checking: Valgrind on dev builds for detailed leak stack traces. ASAN's leak sanitizer catches the majority during automated CI.

### Q65: How would you add fuzz testing to the HTTP parser?

Approach using libFuzzer or AFL++:
1. Write a fuzz target that takes arbitrary `const uint8_t* data, size_t size`.
2. Feed it to `HttpParser::feed()` equivalent that wraps `methodFromRequest`, `pathFromRequest`, `bodyFromRequest`, and `checkAuth`.
3. Assert no crash, no out-of-bounds access, no infinite loop.
4. Test edge cases intentionally: oversized headers (>64KB), partial chunk boundaries, invalid UTF-8 in JSON body, negative Content-Length, HTTP/0.9-style requests, pipelined requests with partial reads, embedded null bytes.
5. Run with `-fsanitize=fuzzer,address,undefined` for maximum coverage.

For the JSON parser path specifically, nlohmann/json is already well-fuzzed upstream, but the interaction between HTTP parsing and JSON parsing (truncated bodies, partial JSON) should be tested.

### Q66: How do you test concurrent safety?

- **TSAN (ThreadSanitizer)**: Enabled in CI for debug builds via MiniKV's sanitizer CMake module. Detects data races, lock order inversions, and atomic access violations.
- **HNSWIndex concurrency design**: `std::shared_mutex` 鈥?shared lock for `search()`, exclusive lock for `insert()`. TSAN verifies no unprotected shared state access.
- **Concurrent stress test**: Spawn 4 writer threads + 8 reader threads on a shared HNSWIndex. Writers insert 10,000 vectors. Readers continuously search with random queries. After completion, verify: (a) no crashes, (b) all searches return valid (non-negative distance) results, (c) `element_count_` matches number of successful inserts.
- **MiniKV concurrent tests**: The vendor MiniKV project has its own TSAN-enabled tests for the skip list and DB operations.

### Q67: What's your CI pipeline?

GitHub Actions on `ubuntu-22.04`:
1. **Checkout**: `actions/checkout@v4` with `submodules: recursive`.
2. **Install deps**: `g++-12`, `cmake`, `ninja-build`.
3. **Configure**: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON -DCMAKE_CXX_COMPILER=g++-12`.
4. **Build**: `cmake --build build -j$(nproc)`.
5. **Test**: `ctest --test-dir build --output-on-failure`.

All three repos (DeepVector, MiniKV, SkyNet) have independent CI. DeepVector triggers on every push and PR. The pipeline currently runs Release builds (no ASAN/TSAN in CI yet 鈥?those are run locally during development). Adding a Debug+Sanitizer matrix job is a planned improvement.

### Q68: How do you handle regressions?

- **Performance regression**: A benchmark CI job (planned) that runs `bench_hnsw` and fails the build if P99 search latency increases >20% from the baseline. Baselines are stored as artifacts from the main branch.
- **Correctness regression**: The full test suite runs on every push. Any failing test blocks merging. Tests are designed to be deterministic 鈥?fixed RNG seeds (42, 43, 123) ensure reproducible results.
- **Version compatibility**: Release notes document breaking changes. The binary file format (VectorStore header magic number `0x4C554D454E444220`) prevents accidental incompatible file access.
- **Dependency pinning**: Vendor submodules pin exact commits. nlohmann/json pins `v3.11.3` via FetchContent tag. GoogleTest pins `v1.14.0`.

---

## 13. Debugging & Failure Scenarios

### Q69: What happens if the server crashes mid-write?

**MiniKV WAL**: Guarantees durability. On restart, `DBImpl::recover()` replays WAL records into the MemTable, reconstructing the pre-crash state. SSTables are immutable once written 鈥?partial SSTable writes are detected by magic number mismatch and discarded.

**VectorStore mmap**: Writes to `MAP_SHARED` pages are immediately visible in the page cache but not guaranteed on disk until `msync`. If the process crashes before `msync`, recent writes to the mmap'd region may be lost. Recovery strategy:
1. On load, read the header's `count` field.
2. Verify `count 鈮?capacity` and file size matches expected (`header_size + capacity * dim * 4`).
3. If mismatch: trust the header count, ignore trailing garbage. The count is the source of truth 鈥?it's updated last before `msync`.
4. If the header itself is corrupted (magic number mismatch): the file is abandoned, treated as a new database.

**Mitigation**: Call `msync(MS_SYNC)` after every N inserts (e.g., every 1000) to bound data loss. In the current implementation, `flush()` is called only in the Collection destructor.

### Q70: How do you debug a "recall@10 drops from 99% to 80%" issue?

Step-by-step debugging approach:

1. **Verify `ef_search` unchanged**: Check that `ef_search` wasn't accidentally lowered. A drop from 200 to 16 would cause exactly this symptom.
2. **Check HNSW graph integrity**: Verify `element_count_` matches expected. Check for excessive deleted nodes (soft-deletion zombies) that bloat the graph but don't contribute valid results. If >20% of nodes are deleted, graph connectivity degrades.
3. **Verify distance function**: Run `test_distance.cpp`'s `SIMDvsScalarConsistency` test. If the distance function returns wrong values (e.g., sign inversion, off-by-one in dimension count), HNSW graph edges point to wrong neighbors.
4. **Check for stale neighbor references**: After many deletes, a node may have neighbors that are all deleted. The search traverses these edges fruitlessly. Run `searchLayer` profiling 鈥?count visited nodes per search. Abnormally high visited counts (>ef_search 脳 10) indicate zombie edges.
5. **Profile with perf**: `perf record -g ./lumendb_server` during search workload. Check for unexpected hotspots 鈥?e.g., if `std::function` dispatch overhead is high, the lambda indirection may be the bottleneck.
6. **Compare against brute-force**: Run an exact KNN scan on the same data. If exact results match the old recall, the index is degraded. If exact results also show low recall, the data or distance function is wrong.
7. **Check PQ retraining**: If PQ was retrained on a different data distribution, the distance estimates shift. Compare ADC distances against raw L2 distances for a sample of queries.

### Q71: How do you handle corrupted SSTable files?

MiniKV's `SSTableReader::open()` checks:
1. **Magic number**: First 4 bytes must match `0x4D494E49` ("MINI"). If not, return `nullptr` 鈥?the file is skipped during DB open.
2. **CRC32 per block**: Each data block has a stored CRC32. On read, recompute and compare. If mismatch 鈫?skip the block, return partial results. The corrupted block is logged for investigation.
3. **File size check**: The footer (last 48 bytes) contains metadata. If the file is truncated, the footer won't parse 鈥?the file is discarded.

Background compaction naturally regenerates corrupted files: the compaction process reads all valid blocks from all files, merges them, and writes fresh SSTables. Corrupted blocks are simply not included in the merge. If corruption is widespread (e.g., disk failure), the DB may lose data 鈥?MiniKV doesn't implement Reed-Solomon or replication at the storage layer.

### Q72: What if a vector file is truncated mid-write?

The VectorStore binary format is designed for graceful degradation:

1. **Header integrity**: The 64-byte header contains a magic number (`0x4C554D454E444220`). If the magic matches, the file is recognized. If not, the file is abandoned (treated as empty).
2. **Count as source of truth**: On load, `VectorStore::load()` reads the header's `count` field. It verifies that `count 鈮?capacity` and that the expected file size (`header_size + capacity * (dim * 4 + 8)`) matches the actual file size (accounting for the ID-to-offset mapping).
3. **Truncation tolerance**: If `count < capacity` (expected after a crash during `append`): trust the header count, ignore trailing garbage. The `nextID` allocator finds the first free slot 鈮?0 and continues from there.
4. **Partial write in data region**: If a vector write was in progress, the partial vector occupies the slot but the `count` field may not have been incremented yet. The partial data is harmless 鈥?`nextID` will skip over used-looking slots because the ID-to-offset mapping has that slot's ID (non-zero).

### Q73: How do you diagnose OOM when building HNSW index?

HNSW memory breakdown:
- **Nodes array**: `nodes_.size() 脳 sizeof(HNSWNode)` 鈮?`N 脳 (8 + 4 + 24 + 1 + padding)` 鈮?`N 脳 40` bytes. For 1M vectors: ~40MB.
- **Neighbor lists**: Each node at level `l` has up to `M_max0_` (layer 0) or `M_` (upper layers) neighbors, each stored as `uint64_t`. Average neighbor list size: `M_max0_ + 危(P(level >= l) 脳 M_)` 鈮?`2M + (1/M + 1/M虏 + ...) 脳 M` 鈮?`2M + 1`. With M=16: ~33 neighbors 脳 8 bytes 鈮?264 bytes/node. For 1M: ~264MB.
- **Vector data**: `N 脳 dim 脳 4` bytes. 768-dim: ~3GB raw.
- **Distance scratch buffers**: `M 脳 K 脳 4` bytes for PQ distance table. M=96, K=256: ~96KB. Negligible.
- **Total for 1M vectors, M=16, 768-dim raw**: ~3GB + 40MB + 264MB 鈮?3.3GB.
- **With PQ (M=96)**: ~96MB for codes + same index overhead 鈮?400MB.

Monitoring approach:
1. Read `/proc/self/status` 鈫?`VmRSS` for RSS, `VmSize` for virtual memory.
2. Track `nodes_.capacity()` 脳 `sizeof(HNSWNode)` for vector overhead.
3. Estimate neighbor list memory: sum all `nodes_[i].neighbors[j].size()`.
4. If using mmap: `mmap`'d regions don't count toward RSS until faulted. Use `VmSize` to account for them.
5. Set `max_elements` in `CollectionConfig` to pre-reserve and fail early if the system can't allocate.

### Q74: How do you handle the case where PQ encoding has never been seen before (outlier)?

PQ always maps to the nearest centroid regardless of distance 鈥?there's no "rejection" mechanism. An outlier vector (far from all centroids) gets encoded to the closest centroid, which may still be very far away. This causes distance estimation error: the PQ-estimated distance is larger than the true distance, leading to false negatives in search.

Mitigations:
1. **Train PQ on representative data**: Ensure the training set covers the expected query distribution. Use a large diverse training set (10K+ vectors from production data, not synthetic).
2. **Residual quantization (RQ)**: Before PQ, apply a coarse quantizer (e.g., IVF with N centroids). Encode the residual (vector - coarse centroid) with PQ. The coarse quantizer handles the "which region" question, and PQ only needs to model local variance.
3. **Combine with IVF**: IVF restricts the search space to the nearest C clusters. Outliers only need to be compared against vectors in their cluster, reducing the impact of cross-cluster distance errors.
4. **Overlap search**: For critical queries, run a raw L2 distance verification pass on the top 2脳k PQ candidates. This catches false negatives caused by distance estimation error at the cost of O(dim 脳 2k) extra computation.
5. **Monitor reconstruction error**: Track the mean squared error between raw L2 distance and PQ-estimated distance during search. If it spikes, retrain PQ on newer data.

---

## 14. System Integration

### Q75: How would you integrate DeepVector with OpenAI embeddings?

```python
import numpy as np
from openai import OpenAI
from lumendb import Collection, CollectionConfig, DocumentMeta

client = OpenAI(api_key="sk-...")
cfg = CollectionConfig()
cfg.dim = 1536  # text-embedding-ada-002
cfg.metric = deepvector.DistanceMetric.Cosine
coll = Collection(cfg, "/data/lumendb_openai")

def embed(text: str) -> np.ndarray:
    resp = client.embeddings.create(model="text-embedding-ada-002", input=text)
    return np.array(resp.data[0].embedding, dtype=np.float32)

# Ingest documents
documents = ["DeepVector is a vector database", "RAG uses retrieval + generation"]
for doc in documents:
    vec = embed(doc)
    meta = DocumentMeta()
    meta.text = doc
    meta.timestamp = int(time.time())
    coll.add_with_meta(vec, meta)

# Search
query_vec = embed("What is DeepVector?")
results = coll.search(query_vec, k=3)
for r in results:
    meta = coll.get_meta(r.id)
    print(f"[{r.distance:.4f}] {meta.text}")
```

For LangChain users, the built-in `DeepVectorVectorStore` wraps this pattern 鈥?just pass `OpenAIEmbeddings()` as the embedding function and use `add_texts`/`similarity_search`.

### Q76: How do you handle multi-tenancy?

Strategy 1 鈥?**Separate collections per tenant**:
```cpp
auto coll_tenant_a = std::make_unique<Collection>(cfg, "/data/tenant_a");
auto coll_tenant_b = std::make_unique<Collection>(cfg, "/data/tenant_b");
```
Each tenant gets its own `vectors.bin` and `docs/` directory. Isolation is at the filesystem level. Trade-off: many open file descriptors, no shared memory across tenants.

Strategy 2 鈥?**Tenant ID in metadata**:
```cpp
meta.tags = "tenant:a";  // or use a dedicated tenant_id field added to DocumentMeta
coll.add_with_meta(vec, meta);
```
Filter every search with `FilterNode::eq("tenant_id", tenant)`. Single collection, simpler deployment, but vectors from different tenants live in the same HNSW graph 鈥?the graph traversal visits all vectors, only filtering at the end. This wastes time on cross-tenant distance computations.

Strategy 3 鈥?**JWT-based routing**: The HTTP server extracts `tenant_id` from the JWT claim and routes to the correct Collection instance. Requires a `map<tenant_id, Collection>` on the server side. Auth is handled at the HTTP layer, not in the database.

Recommendation: Strategy 1 for strong isolation, Strategy 3 for SaaS deployments.

### Q77: How do you do A/B testing with different index configurations?

Shadow deployment approach:
1. **Deploy two DeepVector instances**: Instance A (production) with current config (M=16, ef_search=50, raw). Instance B (experiment) with candidate config (M=32, ef_search=100, PQ).
2. **Dual-write**: All inserts go to both instances.
3. **Shadow-read**: For N% of queries (e.g., 10%), search both instances. Compare results:
   - Recall overlap: `|results_A 鈭?results_B| / k`
   - Latency: P50/P99 for each instance.
   - Distance distribution: Are results from B consistently farther (worse quality)?
4. **Metrics**: Log `{config: "A", recall: 0.99, p99_ms: 0.8}` and `{config: "B", recall: 0.97, p99_ms: 0.4}` to your metrics system.
5. **Decision**: If B meets recall SLA and improves latency/memory, promote B to production. Archive A's config for rollback.

In-process approach (for library use): Create two `Collection` instances with different `CollectionConfig`s. Insert into both. Search both. Compare. This works well for offline evaluation.

### Q78: How do you monitor vector quality drift in production?

Drift indicators:
1. **Reconstruction error (PQ only)**:
   ```
   drift_score = avg(|raw_l2_dist - pq_est_dist|) over last 1000 queries
   ```
   If the ratio of PQ distance to raw distance shifts >10%, retrain PQ codebooks.

2. **Distance distribution shift**:
   ```
   Track P50 and P99 of search result distances over time.
   If P50 shifts from 0.3 to 0.6 鈫?data distribution has changed significantly.
   ```

3. **Recall@1 self-search**:
   Periodically select a random inserted vector, search it against itself, verify it's the first result. If self-recall drops, the index has degraded (excessive soft-deletes, PQ drift, or graph corruption).

4. **Cluster drift detection**:
   Periodically compute centroid of the last 1000 inserted vectors and compare to global centroid. If cosine distance > threshold (e.g., 0.1), alert 鈥?the distribution is changing.

5. **Implement via a metrics endpoint**:
   Expose these metrics via `/stats` or `/metrics`:
   ```
   lumendb_drift_reconstruction_error 0.05
   lumendb_drift_distance_p50 0.32
   lumendb_drift_self_recall 0.995
   ```
   Alert if any metric crosses a threshold (e.g., self-recall < 0.98, reconstruction error > 0.15).
