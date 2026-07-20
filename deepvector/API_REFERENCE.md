# DeepVector API Reference

Complete reference for C++, HTTP, and Python APIs.

---

## C++ API

### Headers

```cpp
#include <dv/deepvector.h>        // Includes types.h + collection.h
#include <dv/filter.h>         // FilterNode, evaluateFilter
#include <dv/server/server.h>  // DeepVectorServer, ServerConfig, ServerStats
#include <dv/storage/document_store.h> // DocumentMeta
```

### CollectionConfig

Configuration for `Collection`. All fields have sensible defaults.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `dim` | `Dimension` (uint32_t) | `0` | Vector dimension. Must be set before creating Collection. |
| `metric` | `DistanceMetric` | `L2` | Distance metric: `L2`, `InnerProduct`, or `Cosine`. |
| `hnsw_m` | `size_t` | `16` | Max neighbors per node (layer 1+). Layer 0 uses `2 脳 hnsw_m`. Higher = better recall, more memory. |
| `hnsw_ef_construction` | `size_t` | `200` | Beam width during index construction. Higher = better graph quality, slower insertion. |
| `hnsw_ef_search` | `size_t` | `50` | Beam width during search. Higher = better recall, slower search. Can be changed at search time. |
| `max_elements` | `size_t` | `0` | Pre-reserve capacity. `0` = unlimited (grow as needed). |
| `use_pq` | `bool` | `false` | Enable Product Quantization. Auto-trains after 256 inserts. |
| `pq_M` | `size_t` | `0` | Number of subspaces for PQ. `0` = auto (`dim / 4`). |
| `pq_K` | `size_t` | `256` | Codebook size per subspace. Must be power of 2. |
| `use_sq` | `bool` | `false` | Enable Scalar Quantization (int8). Auto-trains after 100 inserts. |

### DistanceMetric

```cpp
enum class DistanceMetric : uint8_t {
    L2 = 0,          // Euclidean distance: sqrt(sum((a-b)^2))
    InnerProduct = 1, // Negative dot product: -sum(a*b)
    Cosine = 2,       // Cosine distance: 1 - dot(a,b) / (|a|*|b|)
};
```

- **L2**: Standard Euclidean. Always non-negative. Good default.
- **InnerProduct**: Returns `-dot(a,b)`. Use when vectors are unnormalized and you want maximum inner product search. To use as "maximum cosine similarity", normalize vectors first and use InnerProduct.
- **Cosine**: Returns `1 - cosine_similarity`. Range [0, 2]. Requires computing norms at query time for each candidate 鈥?slightly slower than L2 for queries (but norms can be pre-stored for stored vectors).

### SearchResult

```cpp
struct SearchResult {
    VectorID id;       // uint64_t 鈥?1-based vector ID (0 = invalid)
    float distance;    // Distance according to the configured metric
};
```

### DocumentMeta

```cpp
struct DocumentMeta {
    std::string text;       // Document text content
    std::string tags;       // Comma-separated tags or arbitrary string
    int64_t timestamp;      // Unix timestamp or any integer metadata
};
```

### Collection

Main entry point. Manages vectors, HNSW index, metadata, and quantization.

```cpp
class Collection {
public:
    // Constructor 鈥?opens or creates data at data_dir.
    // If data_dir doesn't exist, creates it. If vectors.bin exists, loads it.
    explicit Collection(const CollectionConfig& config,
                        const std::string& data_dir = ".");
    ~Collection();  // Flushes mmap and closes MiniKV on destruction.

    // Insert a vector. Returns the assigned ID (1-based, monotonically increasing).
    // ID 0 is reserved as invalid.
    uint64_t add(const float* vector);

    // Insert with metadata. Metadata is stored in MiniKV keyed by vector ID.
    uint64_t add(const float* vector, const storage::DocumentMeta& meta);

    // Soft-delete a vector. ID may be reused for future inserts.
    void remove(uint64_t id);

    // K-nearest-neighbor search. Returns up to k results, sorted by distance asc.
    // Thread-safe: multiple concurrent searches can proceed (shared_lock).
    std::vector<SearchResult> search(const float* query, size_t k = 10) const;

    // Search with a filter. Progressively widens ef_search to find enough
    // candidates that match the filter. Returns at most k results.
    // Filter evaluation is O(M) per candidate for simple filters.
    std::vector<SearchResult> searchWithFilter(const float* query, size_t k,
                                                const FilterNode& filter) const;

    // Zero-copy access to a vector. Returns pointer into the mmap'd region.
    // Returns nullptr if id is invalid or deleted.
    // WARNING: The pointer becomes invalid after any operation that grows the store.
    const float* getVector(uint64_t id) const;

    // Retrieve metadata for a vector.
    std::optional<storage::DocumentMeta> getMeta(uint64_t id) const;

    // Number of active (non-deleted) vectors in the collection.
    size_t size() const;

    // Configured dimension.
    Dimension dim() const;

    // Flush mmap data to disk (msync). Called automatically by destructor.
    // Call explicitly after bulk insert if you need durability before shutdown.
    void save(const std::string& name);

    // Load a collection by name. Currently returns nullptr 鈥?implementation pending.
    static std::unique_ptr<Collection> load(const std::string& name,
                                            const std::string& data_dir = ".");
};
```

#### Example: Basic Usage

```cpp
#include <dv/collection.h>
using namespace lumendb;

CollectionConfig cfg;
cfg.dim = 768;
cfg.metric = DistanceMetric::Cosine;

Collection coll(cfg, "./my_data");

std::vector<float> v(768, 0.1f);
uint64_t id = coll.add(v.data());

auto results = coll.search(v.data(), 10);
for (auto& r : results) {
    printf("id=%llu dist=%.4f\n", r.id, r.distance);
}
```

#### Example: With PQ Compression

```cpp
CollectionConfig cfg;
cfg.dim = 768;
cfg.use_pq = true;
cfg.pq_M = 96;  // 768/8 = 96 subspaces, 8-dim each
cfg.hnsw_m = 16;

Collection coll(cfg, "./pq_data");

// Insert 256+ vectors to trigger automatic PQ training
for (int i = 0; i < 1000; ++i) {
    coll.add(random_vector.data());
}
// After training, all subsequent inserts and searches use PQ compression
auto results = coll.search(query.data(), 10);
```

### FilterNode

AST-based filter expressions for metadata search.

```cpp
enum class FilterOp : uint8_t {
    Equals = 0,      // field == value
    NotEquals = 1,   // field != value
    GreaterThan = 2, // field > value (numeric comparison attempted first)
    LessThan = 3,    // field < value
    GreaterEqual = 4,// field >= value
    LessEqual = 5,   // field <= value
    Contains = 6,    // field contains value (substring)
    And = 10,        // Logical AND (all children must pass)
    Or = 11,         // Logical OR (any child must pass)
    Not = 12,        // Logical NOT (negates first child)
};

struct FilterNode {
    FilterOp op;
    std::string field;    // Metadata field name
    std::string value;    // Comparison value
    std::vector<FilterNode> children;  // For And/Or/Not nodes

    bool isLeaf() const;  // op < FilterOp::And

    // Static constructors for leaf nodes
    static FilterNode eq(const std::string& field, const std::string& value);
    static FilterNode contains(const std::string& field, const std::string& value);
    static FilterNode gt(const std::string& field, const std::string& value);
    static FilterNode lt(const std::string& field, const std::string& value);

    // Static constructors for composite nodes
    static FilterNode andAlso(FilterNode a, FilterNode b);
    static FilterNode orElse(FilterNode a, FilterNode b);
};
```

Available metadata fields for filtering:
- `"text"` 鈥?Document text content
- `"tags"` 鈥?Comma-separated tags
- `"timestamp"` 鈥?Integer timestamp (supports numeric comparison)

#### Example: Filter Expressions

```cpp
// Simple equality
auto f1 = FilterNode::eq("tags", "urgent");

// Substring match
auto f2 = FilterNode::contains("text", "vector database");

// Numeric comparison
auto f3 = FilterNode::gt("timestamp", "1700000000");

// AND combination
auto f4 = FilterNode::andAlso(
    FilterNode::eq("tags", "science"),
    FilterNode::gt("timestamp", "1000000")
);

// OR combination
auto f5 = FilterNode::orElse(
    FilterNode::eq("tags", "urgent"),
    FilterNode::eq("tags", "important")
);

// NOT (negation)
// Note: FilterNode has no static `not_` constructor in public API.
// Use And/Or with inverted comparison instead.
auto f6 = FilterNode::eq("tags", "spam");  // "not spam" = filter where tags != spam
                                          // Use searchWithoutFilter and manually exclude.

// Complex: (tags contains "tech" AND timestamp > 1000) OR tags == "urgent"
auto complex = FilterNode::orElse(
    FilterNode::andAlso(
        FilterNode::contains("tags", "tech"),
        FilterNode::gt("timestamp", "1000")
    ),
    FilterNode::eq("tags", "urgent")
);
```

### ServerConfig

```cpp
struct ServerConfig {
    std::string host = "0.0.0.0";   // Bind address
    int port = 8080;                // Listen port
    size_t num_threads = 4;         // Reserved for future thread pool
    size_t max_connections = 10000; // FD limit hint
    std::string data_dir = "./lumendb_data";  // Data storage directory
    std::string api_key = "";       // Bearer token. Empty = no auth.
};
```

### ServerStats

```cpp
struct ServerStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> search_requests{0};
    std::atomic<uint64_t> insert_requests{0};
    std::atomic<uint64_t> error_requests{0};
    std::atomic<uint64_t> active_connections{0};
};
```

### DeepVectorServer

```cpp
class DeepVectorServer {
public:
    // Takes ownership of the Collection.
    explicit DeepVectorServer(const ServerConfig& config,
                           std::unique_ptr<Collection> collection);
    ~DeepVectorServer();  // Calls stop(), then destructs Collection (which flushes).

    // Start the server thread. Non-blocking 鈥?returns immediately.
    void start();

    // Stop the server thread. Blocks until server thread exits.
    // Closes all client connections.
    void stop();

    // Get current stats. Thread-safe (atomic counters).
    const ServerStats& stats() const;
};
```

#### Example: Embedded Server

```cpp
#include <dv/collection.h>
#include <dv/server/server.h>
#include <csignal>

using namespace lumendb;
using namespace dv::server;

std::atomic<bool> running{true};
void handle_signal(int) { running = false; }

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    ServerConfig srv_cfg;
    srv_cfg.port = 9999;
    srv_cfg.data_dir = "./server_data";
    srv_cfg.api_key = "secret";  // Optional

    CollectionConfig coll_cfg;
    coll_cfg.dim = 768;
    coll_cfg.metric = DistanceMetric::Cosine;

    auto coll = std::make_unique<Collection>(coll_cfg, srv_cfg.data_dir);
    DeepVectorServer server(srv_cfg, std::move(coll));
    server.start();

    std::cout << "Server running on port " << srv_cfg.port << std::endl;
    while (running) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server.stop();
    return 0;
}
```

---

## HTTP API

Base URL: `http://{host}:{port}`

All endpoints except `/health` require authentication if `api_key` is configured.

### Authentication

```
Header: Authorization: Bearer <api_key>
```

If `api_key` is empty (default), no authentication is required on any endpoint. If configured, all non-health endpoints return `401 Unauthorized` when the header is missing or incorrect.

### GET /health

Liveness check. No authentication required.

**Response** `200 OK`:
```json
{
    "status": "ok",
    "vectors": 12345,
    "dim": 768
}
```

### GET /stats

Server statistics.

**Response** `200 OK`:
```json
{
    "requests": 15023,
    "searches": 12000,
    "inserts": 3000,
    "errors": 23,
    "connections": 2
}
```

### POST /search

K-nearest-neighbor search.

**Request**:
```json
{
    "vector": [0.1, 0.2, 0.3, ..., 0.768],
    "k": 10,
    "filter": {
        "op": "eq",
        "field": "tags",
        "value": "important"
    }
}
```

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `vector` | float[] | Yes | 鈥?| Query vector, length must match collection dim |
| `k` | integer | No | `10` | Number of results to return |
| `filter` | object | No | 鈥?| Filter expression (see Filter Format below) |

**Filter Format**:

Leaf filter:
```json
{"op": "eq",       "field": "tags", "value": "science"}
{"op": "contains", "field": "text", "value": "database"}
{"op": "gt",       "field": "timestamp", "value": "1000000"}
{"op": "lt",       "field": "timestamp", "value": "2000000"}
```

Composite filter:
```json
{
    "op": "and",
    "children": [
        {"op": "contains", "field": "tags", "value": "tech"},
        {"op": "gt", "field": "timestamp", "value": "1000000"}
    ]
}
```

```json
{
    "op": "or",
    "children": [
        {"op": "eq", "field": "tags", "value": "urgent"},
        {"op": "eq", "field": "tags", "value": "important"}
    ]
}
```

**Response** `200 OK`:
```json
{
    "results": [
        {"id": 42, "distance": 0.1234},
        {"id": 17, "distance": 0.5678}
    ]
}
```

### POST /insert

Insert one or more vectors.

**Single insert**:
```json
{
    "vector": [0.1, 0.2, 0.3, 0.4]
}
```

**Batch insert**:
```json
{
    "vectors": [
        [1.0, 0.0, 0.0],
        [0.0, 1.0, 0.0],
        [0.0, 0.0, 1.0]
    ]
}
```

**Response** `200 OK`:
```json
{
    "ids": [1, 2, 3]
}
```

Note: The current HTTP server does not support inserting with metadata. Use the C++ or Python API for metadata.

### DELETE /vectors/:id

Soft-delete a vector.

**Response** `200 OK`:
```json
{
    "status": "ok"
}
```

### Error Responses

```json
// 400 Bad Request
{"error": "bad request"}

// 400 Bad Request (parse error)
{"error": "[json.exception.parse_error.101] ..."}

// 401 Unauthorized
{"error": "unauthorized"}

// 404 Not Found
{"error": "not found"}

// 405 Method Not Allowed
{"error": "not found"}  // Currently returns 404 for any unrecognized path
```

---

## Python API

```python
import lumendb
import numpy as np
```

### Classes

All classes mirror the C++ API. Types and signatures:

```python
class DistanceMetric(enum.IntEnum):
    L2 = 0
    InnerProduct = 1
    Cosine = 2

class SearchResult:
    id: int        # uint64_t
    distance: float

class DocumentMeta:
    text: str
    tags: str
    timestamp: int  # int64_t

class CollectionConfig:
    dim: int              # uint32_t, required
    metric: DistanceMetric  # default L2
    hnsw_m: int           # default 16
    hnsw_ef_construction: int  # default 200
    hnsw_ef_search: int   # default 50
    max_elements: int     # default 0 (unlimited)
    use_pq: bool          # default False
    pq_M: int             # default 0 (auto: dim/4)
    pq_K: int             # default 256
    use_sq: bool          # default False

class Collection:
    def __init__(self, config: CollectionConfig, data_dir: str = ".")
    def add(self, vector: np.ndarray) -> int
    def add_with_meta(self, vector: np.ndarray, meta: DocumentMeta) -> int
    def search(self, query: np.ndarray, k: int = 10) -> list[SearchResult]
    def search_with_filter(self, query: np.ndarray, k: int, filter: FilterNode) -> list[SearchResult]
    def get_vector(self, id: int) -> np.ndarray | None   # Zero-copy view
    def get_meta(self, id: int) -> DocumentMeta | None
    def remove(self, id: int) -> None
    def save(self, name: str) -> None
    @staticmethod
    def load(name: str, data_dir: str = ".") -> Collection | None
    def __len__(self) -> int
    dim: int   # property

class FilterNode:
    op: int        # FilterOp enum value
    field: str
    value: str
    children: list[FilterNode]
    def is_leaf(self) -> bool
    @staticmethod
    def eq(field: str, value: str) -> FilterNode
    @staticmethod
    def contains(field: str, value: str) -> FilterNode
    @staticmethod
    def gt(field: str, value: str) -> FilterNode
    @staticmethod
    def lt(field: str, value: str) -> FilterNode
    @staticmethod
    def and_(a: FilterNode, b: FilterNode) -> FilterNode
    @staticmethod
    def or_(a: FilterNode, b: FilterNode) -> FilterNode
```

### NumPy Type Annotations

All vector parameters accept `np.ndarray[np.float32]` with C-contiguous layout. The bindings use `forcecast` to auto-convert compatible dtypes (float64 鈫?float32), but passing `np.float32` directly avoids a copy.

```python
# Preferred: float32, no conversion needed
vec = np.random.randn(768).astype(np.float32)
coll.add(vec)

# Works but copies: float64 鈫?float32 conversion
vec = np.random.randn(768)  # float64
coll.add(vec)  # implicit conversion

# Fails: non-contiguous
vec = some_array[:, ::2]  # non-contiguous stride
coll.add(vec)  # may fail or copy

# Works: make contiguous first
coll.add(np.ascontiguousarray(vec))
```

### Example: Python Full Workflow

```python
import numpy as np
import lumendb

cfg = deepvector.CollectionConfig()
cfg.dim = 768
cfg.metric = deepvector.DistanceMetric.Cosine
cfg.hnsw_m = 32
cfg.hnsw_ef_search = 100

coll = deepvector.Collection(cfg, "/tmp/lumendb_demo")

# Insert with metadata
for i in range(100):
    vec = np.random.randn(cfg.dim).astype(np.float32)
    vec /= np.linalg.norm(vec)

    meta = deepvector.DocumentMeta()
    meta.text = f"Document {i}: some content here"
    meta.tags = "important" if i % 5 == 0 else "normal"
    meta.timestamp = 1000000 + i

    coll.add_with_meta(vec, meta)

# Search
query = np.random.randn(cfg.dim).astype(np.float32)
results = coll.search(query, k=5)

for r in results:
    meta = coll.get_meta(r.id)
    vec = coll.get_vector(r.id)
    print(f"id={r.id} dist={r.distance:.4f} text='{meta.text}'")

# Filtered search
f = deepvector.FilterNode.and_(
    deepvector.FilterNode.eq("tags", "important"),
    deepvector.FilterNode.gt("timestamp", "1000040")
)
filtered = coll.search_with_filter(query, k=5, filter=f)
print(f"Filtered: {len(filtered)} results")
```

---

## Performance Tuning Guide

### HNSW Parameters

| Parameter | Lower Value | Higher Value | Guidance |
|-----------|-------------|-------------|----------|
| `hnsw_m` | Less memory, lower recall | Better recall, more memory | Start at 16. Double for each 10脳 increase in dataset size. |
| `hnsw_ef_construction` | Faster inserts | Better graph quality | Set to 2脳 `ef_search` for production. Set lower for bulk loading. |
| `hnsw_ef_search` | Faster queries, lower recall | Better recall, slower | Set to `k 脳 4` as baseline, increase until recall meets SLA. |

### ef_search Tuning Table (M=16, 768-dim, 1M vectors)

| ef_search | P50 Latency | P99 Latency | Recall@10 | Memory Impact |
|-----------|-------------|-------------|-----------|---------------|
| 16 | 80 碌s | 250 碌s | 92% | None |
| 50 | 150 碌s | 500 碌s | 97% | None |
| 100 | 250 碌s | 800 碌s | 98.5% | None |
| 200 | 450 碌s | 1.5 ms | 99.2% | None |
| 400 | 800 碌s | 3.0 ms | 99.5% | None |

### PQ Configuration Table (768-dim)

| pq_M | Subspace Dim | Compression | Code Memory/Vector | Recall@10 (est.) |
|------|-------------|-------------|---------------------|-------------------|
| 48 | 16 | 48脳 | 48 bytes | 99% |
| 96 | 8 | 32脳 | 96 bytes | 97% |
| 192 | 4 | 16脳 | 192 bytes | 95% |
| 384 | 2 | 8脳 | 384 bytes | 92% |

Higher `pq_M` = more subspaces = higher compression = lower recall. The subspace dimension (`dim / pq_M`) determines the granularity of quantization. Very small subspaces (2-dim) lose too much information.

### SQ Configuration

| Parameter | Value | Compression | Quality Loss |
|-----------|-------|-------------|-------------|
| Per-dim int8 | 鈥?| 4脳 | <1% recall loss for most datasets |

SQ is nearly lossless for vectors with values distributed within a bounded range. Works well for normalized embeddings (all values in [-1, 1]).

### Combined Tuning: PQ + ef_search

Choose `ef_search` to compensate for PQ recall loss:

```
Target recall = PQ_base_recall 脳 (1 - ef_penalty)
```

| pq_M | Base Recall | ef_search for 99% | ef_search for 95% |
|------|-------------|--------------------|--------------------|
| 48 | 99% | 50 | 16 |
| 96 | 97% | 200 | 50 |
| 192 | 95% | 400 | 100 |
