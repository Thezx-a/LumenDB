# TitanKV 实战课程 · 中文大纲

> 面向 C++ 后端 / 分布式系统求职者，从基础语法到分布式全栈，逐层扩展，紧扣 TitanKV 项目源码。

## 阅读约定

每个模块统一结构：

1. **核心知识**：本模块必须掌握的概念清单。
2. **内容详解**：结合 TitanKV 源码的深度讲解，含图示与代码引用。
3. **思考题**：概念辨析与原理追问，检验理解深度。
4. **动手题**：编码实践，对应项目源码或 LeetCode 题目。
5. **自检**：关键词填空 / 判断题，快速检验掌握程度。

代码引用格式：[skip_list.h](file:///c:/Users/Administrator/Desktop/hellocpp/minikv/src/core/skip_list.h)（可点击跳转）。

---

## 学习路径

```
基础层 (Module 01-04)
  └─ C++ 语法 → 现代 C++/并发 → Go/TS
        ↓
数据结构层 (Module 05-06)
  └─ 跳表 → 布隆过滤器/哈希
        ↓
存储引擎层 (Module 07-08)
  └─ LSM-Tree → Compaction/MVCC
        ↓
网络层 (Module 09-10)
  └─ epoll/协程 → HTTP/反向代理
        ↓
分布式层 (Module 11)
  └─ Raft → 分片
        ↓
应用层 (Module 12)
  └─ Go 微服务 → Next.js 控制台
        ↓
面试层 (Module 13)
  └─ 系统设计 + 50+ 真题汇总
```

## 模块清单

### 第一部分：基础篇

- **Module 01 — 环境搭建与项目概览**：CMake/Ninja/GCC 12+/Go 1.23/Node 20+/Docker，编译运行 minikv 与 skynet，理解 TitanKV 整体架构与重构计划。
- **Module 02 — C++ 核心语法**：类型系统、指针/引用、函数重载、命名空间、编译模型、Slice/Status 等项目惯用法。
- **Module 03 — 现代 C++ 与并发**：智能指针、移动语义、lambda、`constexpr`、`std::thread`/`mutex`/`atomic`/`shared_mutex`，对照 SkipList 的读写锁实现。
- **Module 04 — Go 与 TypeScript 基础**：Go goroutine/channel/gRPC，TypeScript/Next.js App Router，为分布式层与控制台铺垫。

### 第二部分：数据结构篇

- **Module 05 — 跳表与有序结构**：跳表概率平衡原理、随机层数、复杂度证明、与红黑树/B+ 树对比，手撕跳表（对应 minikv MemTable）。
- **Module 06 — 布隆过滤器与哈希**：位图 + k 哈希、误判率公式、参数推导、Counting BF、一致性哈希环与虚拟节点。

### 第三部分：存储引擎篇

- **Module 07 — LSM-Tree 存储引擎**：WAL/MemTable/SSTable 文件格式、写路径与读路径、Bloom Filter 集成、Block Cache。
- **Module 08 — Compaction 与 MVCC**：Leveled vs Tiered、写/读/空间放大、InternalKey 编码、Manifest 持久化、崩溃恢复。

### 第四部分：网络篇

- **Module 09 — epoll 与 C++20 协程**：IO 多路复用对比、LT/ET、Reactor 模式、`co_await`/`promise_type`/对称转移、Executor 调度。
- **Module 10 — HTTP 与反向代理**：HTTP/1.1 状态机解析、Router、连接池、负载均衡（轮询/加权/最少连接）、健康检查。

### 第五部分：分布式篇

- **Module 11 — Raft 共识与分片**：Leader 选举、日志复制、安全性、Snapshot、PreVote、一致性哈希分片与在线再均衡。

### 第六部分：应用篇

- **Module 12 — Go 微服务与 Next.js 控制台**：Gin 网关、JWT/RBAC、gRPC 服务、Next.js + TanStack Query 实时仪表盘。

### 第七部分：面试篇

- **Module 13 — 系统设计与面试题汇总**：设计 KV 存储/分布式锁/限流器 + 50+ 真题（LeetCode 1206/146/460、牛客面经、手撕跳表/LRU/线程池/智能指针/epoll 服务器）。

---

## 环境要求速览

| 工具 | 版本 | 用途 |
|---|---|---|
| GCC / Clang | 12+ / 15+ | C++17/20 编译 |
| CMake | 3.20+ | 构建系统 |
| Ninja | 1.11+ | 加速构建（推荐） |
| Go | 1.23+ | 微服务 |
| Node.js | 20+ | Next.js 控制台 |
| Docker | 24+ | 本地开发栈（Postgres/Redis/etcd/Jaeger/Prometheus/Grafana） |
| Python | 3.10+ | minikv Python 客户端（可选） |

## 快速开始

```bash
# C++ 构建与测试
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure

# 启动本地开发栈
docker compose -f deploy/dev/docker-compose.yml up -d

# 统一入口
make help    # 查看所有目标
make build   # 构建 C++ + Go
make test    # 运行测试
```

## 下一步

进入 [Module 01 — 环境搭建与项目概览](./01-overview.md) 开始学习。
