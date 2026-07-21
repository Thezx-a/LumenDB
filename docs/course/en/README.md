# TitanKV Hands-on Course · English Syllabus

> For C++ backend / distributed systems job seekers. From language basics to a full-stack distributed system, layer by layer, tightly bound to the TitanKV source.

## Reading Conventions

Every module follows the same structure:

1. **Core Knowledge** — concepts you must master in this module.
2. **Deep Dive** — explanation grounded in TitanKV source, with diagrams and code references.
3. **Thinking Questions** — conceptual analysis to test depth of understanding.
4. **Hands-on Exercises** — coding practice tied to project source or LeetCode problems.
5. **Self-Check** — fill-in-the-blank / true-false to quickly verify mastery.

Code references look like: [skip_list.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h) (clickable).

---

## Learning Path

```
Foundation (Module 01-04)
  └─ C++ syntax → Modern C++/Concurrency → Go/TS
        ↓
Data Structures (Module 05-06)
  └─ SkipList → BloomFilter/Hashing
        ↓
Storage Engine (Module 07-08)
  └─ LSM-Tree → Compaction/MVCC
        ↓
Networking (Module 09-10)
  └─ epoll/Coroutines → HTTP/Reverse Proxy
        ↓
Distributed (Module 11)
  └─ Raft → Sharding
        ↓
Application (Module 12)
  └─ Go µServices → Next.js Console
        ↓
Interview (Module 13)
  └─ System Design + 50+ real questions
```

## Module List

### Part I: Foundation

- **Module 01 — Env Setup & Project Overview**: CMake/Ninja/GCC 12+/Go 1.23/Node 20+/Docker; build & run minikv and skynet; understand the TitanKV architecture and refactoring plan.
- **Module 02 — C++ Core Syntax**: type system, pointers/references, overloading, namespaces, compilation model, project idioms like Slice/Status.
- **Module 03 — Modern C++ & Concurrency**: smart pointers, move semantics, lambdas, `constexpr`, `std::thread`/`mutex`/`atomic`/`shared_mutex`, contrasted with the SkipList read-write lock.
- **Module 04 — Go & TypeScript Basics**: Go goroutine/channel/gRPC, TypeScript/Next.js App Router — foundation for the distributed layer and console.

### Part II: Data Structures

- **Module 05 — SkipList & Ordered Structures**: probabilistic balancing, random level, complexity proof, comparison with RB-tree/B+tree; hand-write a SkipList (minikv MemTable).
- **Module 06 — BloomFilter & Hashing**: bitmap + k hashes, false-positive formula, parameter derivation, Counting BF, consistent-hash ring & virtual nodes.

### Part III: Storage Engine

- **Module 07 — LSM-Tree Engine**: WAL/MemTable/SSTable file format, write & read paths, Bloom Filter integration, block cache.
- **Module 08 — Compaction & MVCC**: Leveled vs Tiered, write/read/space amplification, InternalKey encoding, Manifest persistence, crash recovery.

### Part IV: Networking

- **Module 09 — epoll & C++20 Coroutines**: IO multiplexing comparison, LT/ET, Reactor pattern, `co_await`/`promise_type`/symmetric transfer, Executor scheduling.
- **Module 10 — HTTP & Reverse Proxy**: HTTP/1.1 state-machine parser, Router, connection pool, load balancing (round-robin/weighted/least-conn), health checks.

### Part V: Distributed

- **Module 11 — Raft & Sharding**: Leader election, log replication, safety, Snapshot, PreVote, consistent-hash sharding & online rebalancing.

### Part VI: Application

- **Module 12 — Go µServices & Next.js Console**: Gin gateway, JWT/RBAC, gRPC services, Next.js + TanStack Query live dashboard.

### Part VII: Interview

- **Module 13 — System Design & Interview Q&A**: design a KV store / distributed lock / rate limiter + 50+ real questions (LeetCode 1206/146/460, NowCoder interview posts, hand-write SkipList/LRU/ThreadPool/SmartPtr/epoll server).

---

## Environment Requirements

| Tool | Version | Purpose |
|---|---|---|
| GCC / Clang | 12+ / 15+ | C++17/20 compilation |
| CMake | 3.20+ | build system |
| Ninja | 1.11+ | faster builds (recommended) |
| Go | 1.23+ | microservices |
| Node.js | 20+ | Next.js console |
| Docker | 24+ | local dev stack (Postgres/Redis/etcd/Jaeger/Prometheus/Grafana) |
| Python | 3.10+ | minikv Python client (optional) |

## Quick Start

```bash
# C++ build & test
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure

# Bring up the local dev stack
docker compose -f deploy/dev/docker-compose.yml up -d

# Unified entry
make help    # list all targets
make build   # build C++ + Go
make test    # run tests
```

## Next Step

Proceed to [Module 01 — Env Setup & Project Overview](./01-overview.md).
