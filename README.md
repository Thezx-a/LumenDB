# TitanKV

> A distributed key-value storage platform built from scratch — storage engine in C++, business layer in Go, console in Next.js.

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17%2F20-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++17/20"/>
  <img src="https://img.shields.io/badge/Go-1.23-00ADD8?style=flat-square&logo=go&logoColor=white" alt="Go"/>
  <img src="https://img.shields.io/badge/Next.js-15-000000?style=flat-square&logo=next.js&logoColor=white" alt="Next.js"/>
  <img src="https://img.shields.io/badge/License-MIT-blue?style=flat-square" alt="MIT"/>
</p>

---

## What is TitanKV?

TitanKV is an end-to-end distributed storage platform, designed and built from the ground up to demonstrate serious backend engineering depth:

- A **self-implemented LSM-Tree storage engine** in C++17 (WAL, MemTable, SSTable, Compaction, Bloom Filter).
- A **C++20 coroutine network library** (SkyNet) for high-concurrency event loops.
- A set of **Go microservices** (API gateway, auth, data, meta, observability) communicating over gRPC.
- A **Raft-based replication layer** (using `hashicorp/raft`) and **consistent-hash sharding**.
- A **Next.js admin console** for managing collections, keys, users, and the cluster.
- A **Go CLI** and **multi-language SDK** for developer integration.
- Kubernetes-native deployment with observability (Prometheus / Grafana / Jaeger).

This is not a wrapper around an existing database. The storage engine, network layer, replication logic, and orchestration are all written here.

---

## Repository Layout

```
titan-kv/
├── storage-engine/   (planned) C++17 LSM-Tree KV engine + gRPC server
├── minikv/           C++17 LSM-Tree core (WAL / MemTable / SST / Compaction)
├── skynet/           C++20 coroutine network library
├── deepvector/       (legacy) C++ HNSW vector index — to be refactored as a secondary index
├── gateway/          (planned) Go API gateway (Gin, auth, rate-limit, routing)
├── services/         (planned) Go microservices (auth / data / meta / observability)
├── distributed/      (planned) Raft replication, sharding, etcd service discovery
├── client-go/        (planned) Go SDK
├── client-cli/       (planned) Cobra CLI tool
├── web/              (planned) Next.js admin console
├── proto/            (planned) gRPC / Protocol Buffers definitions
├── deploy/           Docker Compose dev env / Helm chart / Kubernetes manifests
├── docs/             Architecture and API documentation
├── CMakeLists.txt    Top-level CMake entry
├── go.mod            Go module root
└── Makefile          Unified build / test / lint entry
```

---

## Status

> **Project is being refactored from a previous AI/LLM-focused vector database into a general-purpose distributed KV storage platform.**
>
> See `docs/REFACTORING.md` for the detailed plan and current progress.

| Phase | Description | Status |
|------|-------------|--------|
| Phase 0 | Cleanup + repository restructure | done |
| Phase 1 | C++ storage engine upgrade (MVCC, WAL, compaction, CF) | in-progress (WP 1.2.1 done) |
| Phase 2 | C++ gRPC server + Go cgo client | planned |
| Phase 3 | Go API gateway + auth service (JWT/RBAC/APIKey) | **MVP done** — `gateway/` + `services/auth/` |
| Phase 4 | Go data / meta / observability services | **MVP done** — `services/{data,meta,observability}/` + `client-go/` |
| Phase 5 | Distributed layer: etcd + hashicorp/raft + sharding | planned |
| Phase 6 | Next.js admin console | **MVP done** — `web/` (App Router + TanStack Query + SSE) |
| Phase 7 | Observability + Kubernetes + CI/CD | planned |
| Phase 8 | CLI tool + multi-language SDK + documentation | planned |

> **Note on MVP**: Phase 3/4/6 use in-memory storage (data service) and
> mock metrics (observability). They are fully runnable end-to-end via
> `make run-all` + `make web-dev`. Phase 2 (gRPC server wrapping minikv)
> will replace the in-memory backing store with the real LSM-Tree engine.

---

## Development Environment

### Requirements

- **C++ build:** CMake 3.20+, GCC 12+ (Linux/WSL2), or Clang 15+
- **Go:** 1.23+
- **Node:** 20+ (for the web console)
- **Docker:** 24+ (for the local dev stack)

### Build (C++)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Local development stack

Repositories for PostgreSQL / Redis / etcd / Jaeger / Prometheus / Grafana:

```bash
docker compose -f deploy/dev/docker-compose.yml up -d
```

### Make targets (unified entry)

```bash
make help        # list available targets
make build       # build all C++ + Go services
make test        # run C++ tests + Go tests
make lint        # run clang-tidy + golangci-lint
make docker-up   # bring up the local dev stack
make docker-down # stop the local dev stack

# Phase 3 / 4 — run Go services
make run-auth    # auth service    (port 8082)
make run-data    # data service    (port 8081)
make run-meta    # meta service    (port 8083)
make run-observ  # observability   (port 8084)
make run-gateway # gateway         (port 8080)
make run-all     # all 5 services in parallel

# Phase 6 — Next.js console
make web-install # npm install
make web-dev     # dev server (port 3000)
make web-build   # production build
```

### Quick start (end-to-end)

```bash
# 1. Bring up Redis + etcd (optional, services degrade gracefully without them)
make docker-up

# 2. Run all Go backend services
make run-all

# 3. (new terminal) Run the Next.js console
make web-install && make web-dev

# 4. Open http://localhost:3000 — register at /api/auth/register, login, dashboard.
```

### Course & interview materials

A 13-module bilingual (zh / en) hands-on course is in [`docs/course/`](docs/course/),
covering C++ basics → modern C++/concurrency → SkipList/BloomFilter → LSM-Tree →
epoll/coroutines → HTTP/proxy → Raft/sharding → Go µServices → system design &
interview Q&A (60+ real questions with LeetCode numbers and hand-write skeletons).
Runnable C++ unit tests for the hands-on exercises live in [`tests/course/`](tests/course/).

---

## License

MIT — see [LICENSE](LICENSE).