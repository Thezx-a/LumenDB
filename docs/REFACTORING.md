# =========================================================
# TitanKV — top-level refactor plan (track here)
#
# Each phase is split into work packages (WPs).
# Mark status with: [ ] todo   [~] in-progress   [x] done
# =========================================================

## Phase 0 — Cleanup and restructure

- [x] Remove AI / Python agent layer (`deepvector/agent/`, `deepvector/python/`, examples, course notes)
- [x] Remove AI-flavored docs (ARCHITECTURE, PRODUCTION_QA, OPERATIONS, etc.)
- [x] Keep `deepvector/{src,include,tests,benchmarks,cmake}` (C++ HNSW server) — to be re-organized in Phase 1
- [x] Keep `minikv/` — will become `storage-engine/` core in Phase 1
- [x] Keep `skynet/` — C++20 coroutine network library
- [x] Add top-level skeleton: `gateway/ services/ distributed/ client-go/ client-cli/ web/ proto/ deploy/ docs/`
- [x] New `.gitignore`, `go.mod`, top-level `CMakeLists.txt`, `README.md`, `Makefile`
- [x] `deploy/dev/docker-compose.yml` — local dev stack (Postgres, Redis, etcd, Jaeger, Prometheus, Grafana)

## Phase 1 — C++ storage engine upgrade

- [x] WP 1.2.1  SSTable block compression (Snappy / ZSTD)
      - Added `core/compression.{h,cpp}` with `compressBlock` / `decompressBlock`
      - Added `cmake/FetchCompression.cmake` pulling snappy 1.2.1 + zstd 1.5.6 via FetchContent
      - New on-disk block format: `[crc(4)][physical_size(4)][uncompressed_size(4)][type(1)][payload]`
      - Footer gains 1-byte `format_version` (currently 1)
      - `Options::compression` (uint8_t, default 1=snappy) propagated through `DBImpl::flushMemTable`
      - Tests: `test_compression.cpp` (round-trip, mismatch detection) and `test_sstable_compression.cpp`
      - Status: code-reviewed, awaiting Linux/WSL build verification (Windows native unsupported)
- [ ] WP 1.2.2  MVCC snapshot reads (internal key = user_key | seq | type)
      - Includes a hard prerequisite: rewrite `InternalKey` from hash-of-user-key to a real
        `[user_key | ~seq | type]` encoding so different user keys never collide
- [ ] WP 1.2.3  Range Delete (WriteBatch::deleteRange, MemTable tombstones)
- [ ] WP 1.2.4  Manifest persistence (recover Version on restart)
- [ ] WP 1.2.5  Column Family (per-CF MemTable + SST, shared WAL)
- [ ] WP 1.2.6  Optimistic transactions (Begin/Commit/Rollback, OCC)
- [ ] WP 1.2.7  Configurable compaction strategy (Leveled vs Size-tiered)

## Phase 2 — C++ gRPC server + Go cgo client

- [ ] `proto/keyforge/storage.proto` defines Put/Get/Delete/WriteBatch/Scan/Txn
- [ ] C++ gRPC server wraps `DBImpl`, served by SkyNet-based task dispatch
- [ ] Go gRPC client (generated from proto)
- [ ] Go `client-go/fast` cgo fallback for embedded mode
- [ ] Benchmark: gRPC Get P50 < 500us, cgo Get P50 < 50us

## Phase 3 — Go API gateway + auth service

- [ ] Gin gateway, middleware chain (RequestID / Logger / Recover / RateLimit / Auth / RBAC)
- [ ] Auth service (register/login/refresh, bcrypt, JWT, RBAC)
- [ ] API Key issuance / revocation + Redis-backed middleware
- [ ] Rate limiter (token bucket via Redis Lua)
- [ ] GitHub OAuth2 login

## Phase 4 — Go data / meta / observability + Go SDK

- [ ] Data service (Put/Get/Delete/Batch/Scan SSE)
- [ ] Meta service (Collection CRUD, hot config via etcd watch)
- [ ] Observability service (metrics aggregation, health-rollup)
- [ ] Go SDK `client-go` with typed errors and retries

## Phase 5 — Distributed: etcd + hashicorp/raft + sharding

- [ ] etcd service registration and discovery
- [ ] `hashicorp/raft` node wrapping a local storage-engine
- [ ] FSM Apply / Snapshot / Restore (SST-based snapshots)
- [ ] Linearizable reads via ReadIndex
- [ ] Consistent-hash sharding and online rebalance
- [ ] Failover test: kill leader, verify election < 5s

## Phase 6 — Next.js admin console

- [ ] App structure (App Router, Tailwind, Shadcn UI, TanStack Query)
- [ ] Login + route guard
- [ ] Dashboard (QPS, latency, storage, node status; live via SSE)
- [ ] Data Explorer (browse KV, Scan, inline edit, bulk delete)
- [ ] Collection management
- [ ] Users & roles
- [ ] API Key management
- [ ] Config management
- [ ] Cluster status (Raft topology + log entries)

## Phase 7 — Observability + K8s + CI/CD

- [ ] Prometheus metrics from all services
- [ ] OpenTelemetry tracing across Go + C++
- [ ] Loki / Promtail structured logs
- [ ] Helm Chart (StatefulSet for engine + Raft; Deployment for stateless Gos)
- [ ] GitHub Actions multi-stage CI
- [ ] ArgoCD GitOps (optional)

## Phase 8 — CLI + multi-language SDK + docs

- [ ] `client-cli` Cobra tool (keyforge get/put/scan/cluster/members/admin)
- [ ] TypeScript SDK generated from OpenAPI
- [ ] Python SDK generated from OpenAPI
- [ ] Documentation (ARCHITECTURE, STORAGE_ENGINE, DISTRIBUTED, API, DEPLOYMENT)

## Cross-phase utilities

- [ ] Unified Makefile target (build/test/lint/proto/docker-up)
- [ ] Benchmark regression tracking
- [ ] Repo-rename: GitHub repo `LumenDB` → `TitanKV`