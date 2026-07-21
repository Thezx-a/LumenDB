# TitanKV 实战课程 / TitanKV Hands-on Course

> 从 C++ 基础语法到分布式 KV 存储全栈实战，逐层扩展，紧扣 TitanKV 项目源码。
>
> From C++ basics to a full-stack distributed KV store, layer by layer, tightly bound to the TitanKV source.

[English Syllabus](./en/README.md) | [中文大纲](./zh/README.md)

---

## 课程定位 / Course Positioning

本课程以 **TitanKV**（一个从零实现的分布式 KV 存储平台）为载体，覆盖：

- **存储引擎层**：C++17 LSM-Tree（WAL / MemTable / SSTable / Compaction / BloomFilter / MVCC）
- **网络层**：C++20 协程网络库（epoll / Executor / Task / HTTP / 反向代理）
- **分布式层**：Go 微服务 + Raft 共识 + 一致性哈希分片
- **应用层**：Next.js 管理控制台 + 可观测性（Prometheus / Grafana / Jaeger）

每个模块包含：**核心知识 → 内容详解 → 思考题 → 动手题 → 自检**，并配套中英双语文档。

Each module ships: **Core Knowledge → Deep Dive → Thinking Questions → Hands-on Exercises → Self-Check**, with bilingual docs.

---

## 模块地图 / Module Map

| # | 模块 / Module | 层 / Layer | 对应源码 / Source |
|---|---|---|---|
| 01 | 环境搭建与项目概览 / Env Setup & Project Overview | 基础 | 顶层 `CMakeLists.txt` / `Makefile` / `go.mod` |
| 02 | C++ 核心语法 / C++ Core Syntax | 基础 | `minikv/src/utils/` |
| 03 | 现代 C++ 与并发 / Modern C++ & Concurrency | 基础 | `minikv/src/core/skip_list.h` |
| 04 | Go 与 TypeScript 基础 / Go & TS Basics | 基础 | `go.mod` / `web/` |
| 05 | 跳表与有序结构 / SkipList & Ordered Structures | 数据结构 | `minikv/src/core/skip_list.h` |
| 06 | 布隆过滤器与哈希 / BloomFilter & Hashing | 数据结构 | `minikv/src/core/bloom_filter.h` |
| 07 | LSM-Tree 存储引擎 / LSM-Tree Engine | 存储引擎 | `minikv/src/core/{wal,memtable,sstable}` |
| 08 | Compaction 与 MVCC / Compaction & MVCC | 存储引擎 | `minikv/src/core/{compaction,internal_key}` |
| 09 | epoll 与 C++20 协程 / epoll & C++20 Coroutines | 网络 | `skynet/src/{net,core}` |
| 10 | HTTP 与反向代理 / HTTP & Reverse Proxy | 网络 | `skynet/src/{http,proxy}` |
| 11 | Raft 共识与分片 / Raft & Sharding | 分布式 | `distributed/` (planned) |
| 12 | Go 微服务与 Next.js 控制台 / Go µServices & Next.js | 应用 | `services/` / `web/` |
| 13 | 系统设计与面试题汇总 / System Design & Interview Q&A | 面试 | 全项目 / Whole project |

---

## 如何使用 / How to Use

1. 按模块顺序学习，每个模块先读「核心知识」，再动手完成「动手题」。
2. 动手题对应 TitanKV 真实源码，建议对照阅读并在本地编译运行。
3. 「自检」用于检验掌握程度，先独立作答再对照参考答案。
4. 模块 13 汇总 50+ 道真实面试题（含 LeetCode 题号与手撕题），用于求职冲刺。

## 目录结构 / Layout

```
docs/course/
├── README.md            本文件 / this file
├── zh/                  中文文档
│   ├── README.md
│   ├── 01-overview.md … 13-interview.md
└── en/                  English docs
    ├── README.md
    └── 01-overview.md … 13-interview.md
```
