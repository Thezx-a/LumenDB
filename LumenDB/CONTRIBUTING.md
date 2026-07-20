# Contributing to LumenDB

## Building from Source

```bash
# Clone
git clone --recurse-submodules https://github.com/Thezx-a/LumenDB.git
cd LumenDB

# Configure (Release)
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTS=ON \
  -DCMAKE_CXX_COMPILER=g++-12

# Build
cmake --build build -j$(nproc)

# Test
ctest --test-dir build --output-on-failure
```

Debug build with sanitizers:

```bash
cmake -B build_dbg -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_TESTS=ON \
  -DCMAKE_CXX_COMPILER=g++-12

cmake --build build_dbg -j$(nproc)
```

## Code Style

We follow the **Google C++ Style Guide** with a few relaxations. A `.clang-format` file is provided in the repository root. Run it before committing:

```bash
clang-format -i src/*.cpp src/**/*.cpp include/**/*.h tests/*.cpp
```

Key conventions:
- C++17 standard required (`CMAKE_CXX_STANDARD 17`)
- No C++ exceptions from the library layer (server layer may use them for HTTP error handling)
- Prefer `std::unique_ptr` over raw pointers for ownership
- Use PIMPL for classes that expose platform-specific dependencies (e.g., sockets, mmap)
- Header-only for SIMD dispatch code (`distance.h`)
- `#pragma once` for include guards
- 4-space indentation, 100-column limit

## Running Tests

```bash
# All tests
ctest --test-dir build --output-on-failure

# Single test
ctest --test-dir build -R HNSWTest -V

# With sanitizers (Debug build)
cmake -B build_san -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_TESTS=ON \
  -DCMAKE_CXX_COMPILER=g++-12 \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"

cmake --build build_san -j$(nproc)
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build_san --output-on-failure
```

Test files:
| File | What it tests |
|------|--------------|
| `test_distance.cpp` | L2, inner product, cosine; SIMD vs scalar consistency |
| `test_hnsw.cpp` | HNSW insert, search, recall, empty index |
| `test_collection.cpp` | Collection create, add, search, getVector |
| `test_filter.cpp` | All filter ops: eq, contains, gt, lt, and, or |
| `test_collection_filter.cpp` | Collection-level filtered search with metadata |
| `test_quantize.cpp` | PQ train/encode/decode/distance; SQ train/encode/decode |

## Project Structure

```
LumenDB/
├── include/lumendb/       # Public headers
│   ├── types.h             #   CollectionConfig, SearchResult, DistanceMetric
│   ├── collection.h        #   Collection class
│   ├── filter.h            #   FilterNode, evaluateFilter
│   ├── lumendb.h           #   Umbrella header
│   ├── index/              #   HNSW index, distance kernels
│   ├── quantize/           #   PQ, SQ headers (not public)
│   ├── storage/            #   VectorStore, DocumentStore headers (not public)
│   └── server/             #   ServerConfig, LumenDBServer
├── src/                    # Implementation
│   ├── collection.cpp
│   ├── filter.cpp
│   ├── index/hnsw.cpp
│   ├── quantize/pq.cpp
│   ├── quantize/scalar.cpp
│   ├── storage/vector_store.cpp
│   ├── storage/document_store.cpp
│   └── server/
│       ├── main.cpp         #   Server entry point
│       └── server.cpp       #   LumenDBServer::Impl
├── tests/                   # Google Test unit tests
├── benchmarks/              # Google Benchmark microbenchmarks
├── python/                  # pybind11 bindings + LangChain integration
├── vendor/                  # Git submodules
│   ├── MiniKV/              #   LSM-Tree KV store
│   └── SkyNet/              #   C++20 network framework (future use)
├── cmake/                   # CMake modules
├── .github/workflows/       # CI definition
├── CMakeLists.txt           # Top-level build
├── docker-compose.yml
├── Dockerfile
└── .clang-format
```

## PR Process

1. **Fork** the repository and create a feature branch from `main`.
2. **Write code** following the code style guidelines.
3. **Add tests** for new functionality. All existing tests must continue to pass.
4. **Run clang-format** on changed files.
5. **Open a Pull Request** with a descriptive title and body:
   - What problem does this solve?
   - What approach did you take?
   - Any breaking changes?
   - How did you test?
6. **CI must pass**: GitHub Actions runs build + tests on `ubuntu-22.04`. Fix any failures.
7. **Code review**: At least one maintainer must approve. Address review feedback.
8. **Merge**: Squash-merged into `main`.

## Where to Ask Questions

- **GitHub Issues**: Bug reports, feature requests, and questions about usage.
- **GitHub Discussions**: Longer-form questions, architecture ideas, and community topics.
- **Pull Requests**: For contributing code. See PR process above.

## Areas Open for Contribution

| Area | Difficulty | Description |
|------|-----------|-------------|
| epoll migration | ⭐⭐ | Replace `select()` with epoll in the HTTP server |
| Connection keep-alive | ⭐⭐ | Add persistent connections with idle timeout |
| Rate limiting | ⭐ | Token bucket rate limiter per API key |
| Prometheus /metrics | ⭐ | Add histogram metrics endpoint |
| Heuristic neighbor selection | ⭐⭐⭐ | Replace `selectNeighborsSimple` with heuristic pruning |
| VectorStore compaction | ⭐⭐ | Background thread to compact soft-deleted slots |
| PQ periodic retraining | ⭐⭐⭐ | Detect drift and retrain PQ on recent data |
| ARM NEON SIMD | ⭐⭐ | Add NEON intrinsics path in `distance.h` |
| AVX-512 dispatch | ⭐⭐ | Runtime dispatch for AVX-512 distance kernels |
| Full-text metadata search | ⭐⭐ | Add inverted index for text field search |
| Snapshot/backup | ⭐⭐ | Consistent snapshot without stopping writes |
| gRPC server | ⭐⭐ | gRPC API alongside REST for higher throughput |
| DiskANN-style layout | ⭐⭐⭐ | SSD-optimized graph layout for out-of-RAM datasets |
| Multi-collection server | ⭐ | Support multiple named collections in the HTTP server |
| Delete with metadata | ⭐ | Add metadata support to HTTP insert endpoint |

## License

MIT. See the repository root for the full license text.
