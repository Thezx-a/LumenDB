# Contributing to DeepVector

## Building from Source

```bash
# Clone
git clone --recurse-submodules https://github.com/Thezx-a/DeepVector.git
cd DeepVector

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
DeepVector/
鈹溾攢鈹€ include/lumendb/       # Public headers
鈹?  鈹溾攢鈹€ types.h             #   CollectionConfig, SearchResult, DistanceMetric
鈹?  鈹溾攢鈹€ collection.h        #   Collection class
鈹?  鈹溾攢鈹€ filter.h            #   FilterNode, evaluateFilter
鈹?  鈹溾攢鈹€ deepvector.h           #   Umbrella header
鈹?  鈹溾攢鈹€ index/              #   HNSW index, distance kernels
鈹?  鈹溾攢鈹€ quantize/           #   PQ, SQ headers (not public)
鈹?  鈹溾攢鈹€ storage/            #   VectorStore, DocumentStore headers (not public)
鈹?  鈹斺攢鈹€ server/             #   ServerConfig, DeepVectorServer
鈹溾攢鈹€ src/                    # Implementation
鈹?  鈹溾攢鈹€ collection.cpp
鈹?  鈹溾攢鈹€ filter.cpp
鈹?  鈹溾攢鈹€ index/hnsw.cpp
鈹?  鈹溾攢鈹€ quantize/pq.cpp
鈹?  鈹溾攢鈹€ quantize/scalar.cpp
鈹?  鈹溾攢鈹€ storage/vector_store.cpp
鈹?  鈹溾攢鈹€ storage/document_store.cpp
鈹?  鈹斺攢鈹€ server/
鈹?      鈹溾攢鈹€ main.cpp         #   Server entry point
鈹?      鈹斺攢鈹€ server.cpp       #   DeepVectorServer::Impl
鈹溾攢鈹€ tests/                   # Google Test unit tests
鈹溾攢鈹€ benchmarks/              # Google Benchmark microbenchmarks
鈹溾攢鈹€ python/                  # pybind11 bindings + LangChain integration
鈹溾攢鈹€ vendor/                  # Git submodules
鈹?  鈹溾攢鈹€ MiniKV/              #   LSM-Tree KV store
鈹?  鈹斺攢鈹€ SkyNet/              #   C++20 network framework (future use)
鈹溾攢鈹€ cmake/                   # CMake modules
鈹溾攢鈹€ .github/workflows/       # CI definition
鈹溾攢鈹€ CMakeLists.txt           # Top-level build
鈹溾攢鈹€ docker-compose.yml
鈹溾攢鈹€ Dockerfile
鈹斺攢鈹€ .clang-format
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
| epoll migration | 猸愨瓙 | Replace `select()` with epoll in the HTTP server |
| Connection keep-alive | 猸愨瓙 | Add persistent connections with idle timeout |
| Rate limiting | 猸?| Token bucket rate limiter per API key |
| Prometheus /metrics | 猸?| Add histogram metrics endpoint |
| Heuristic neighbor selection | 猸愨瓙猸?| Replace `selectNeighborsSimple` with heuristic pruning |
| VectorStore compaction | 猸愨瓙 | Background thread to compact soft-deleted slots |
| PQ periodic retraining | 猸愨瓙猸?| Detect drift and retrain PQ on recent data |
| ARM NEON SIMD | 猸愨瓙 | Add NEON intrinsics path in `distance.h` |
| AVX-512 dispatch | 猸愨瓙 | Runtime dispatch for AVX-512 distance kernels |
| Full-text metadata search | 猸愨瓙 | Add inverted index for text field search |
| Snapshot/backup | 猸愨瓙 | Consistent snapshot without stopping writes |
| gRPC server | 猸愨瓙 | gRPC API alongside REST for higher throughput |
| DiskANN-style layout | 猸愨瓙猸?| SSD-optimized graph layout for out-of-RAM datasets |
| Multi-collection server | 猸?| Support multiple named collections in the HTTP server |
| Delete with metadata | 猸?| Add metadata support to HTTP insert endpoint |

## License

MIT. See the repository root for the full license text.
