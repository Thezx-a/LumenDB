<p align="center">
  <img src="https://img.shields.io/badge/C++17-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++17"/>
  <img src="https://img.shields.io/badge/C++20-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++20"/>
  <img src="https://img.shields.io/badge/Python-3.11-3776AB?style=for-the-badge&logo=python" alt="Python 3.11"/>
  <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge" alt="MIT"/>
</p>

<h1 align="center">C++ 向量数据库与 Agent 智能检索 · 全栈项目</h1>

<p align="center">
  <b>MiniKV</b> · <b>SkyNet</b> · <b>LumenDB</b> · <b>AgenticDB</b><br/>
  <i>LSM-Tree KV 存储 · C++20 协程网络 · 零拷贝向量数据库 · LLM Agent 智能检索</i>
</p>

<p align="center">
  <a href="#项目概览">中文</a> •
  <a href="#project-overview">English</a>
</p>

---

```mermaid
mindmap
  root((hellocpp))
    MiniKV
      LSM-Tree 存储引擎
      WAL + MemTable + SST
      Bloom 过滤器
      Compaction
    SkyNet
      C++20 协程
      Task<T> 异步框架
      HTTP Parser/Router
      负载均衡
    LumenDB
      向量数据库
      HNSW 索引
      mmap 零拷贝存储
      PQ/SQ 量化压缩
      pybind11 Python 绑定
    AgenticDB
      LLM Agent 层
      多轮智能检索
      MCP 协议集成
      生产部署文档
```

---

## 项目概览

本项目包含四个紧密关联的 C++/Python 项目，构建了一个完整的向量数据库与 AI 智能检索系统。

| 项目 | 语言 | 说明 | 课程 |
|------|------|------|------|
| **MiniKV** | C++17 | LSM-Tree KV 存储引擎 (WAL, MemTable, SSTable, Compaction) | [课程](./lumendb/course/ch05_lsm_tree/) |
| **SkyNet** | C++20 | 协程网络框架 (Task<T>, HTTP Parser, Load Balancer) | [课程](./lumendb/course/ch11_coroutines/) |
| **LumenDB** | C++17 | 零拷贝向量数据库 (HNSW, mmap, PQ/SQ, pybind11) | [课程](./lumendb/course/) |
| **AgenticDB** | Python | LLM Agent 智能检索层 (多轮搜索, MCP 协议) | [课程](./lumendb/course/#章节列表) |

## 快速开始 / Quick Start

```bash
# 编译 C++ 项目 / Build all C++ projects
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 安装 Python 依赖 / Install Python deps
pip install httpx pydantic sentence-transformers fastapi uvicorn mcp

# 启动 LumenDB 服务器 / Start LumenDB server
./build/lumendb_server --port 8080 --dim 384

# 启动 Agent 层 / Start Agent layer
cd lumendb && python -m agent.server.app
```

## 课程目录 / Course Index

| 课程 | 链接 | 章节数 |
|------|------|--------|
| 🎓 LumenDB C++ 向量数据库 | [course/](./lumendb/course/README.md) | 13 章 |
| 🤖 AgenticDB 智能检索 | [course/ AgenticDB 章节](./lumendb/course/) | 13 章 (+ 5 前置) |

## 文档 / Documentation

| 文档 | 链接 | 内容 |
|------|------|------|
| 📖 架构 | [AGENTICDB.md](./lumendb/docs/AGENTICDB.md) | AgenticDB 系统架构 |
| 🔧 操作手册 | [OPERATIONS.md](./lumendb/docs/OPERATIONS.md) | 安装/配置/排错 |
| 🎯 面试题 | [PRODUCTION_QA.md](./lumendb/docs/PRODUCTION_QA.md) | 生产部署深度问答 |
| 📚 API 参考 | [API_REFERENCE.md](./lumendb/API_REFERENCE.md) | C++/Python/HTTP API |

## 技术栈 / Tech Stack

```mermaid
graph TB
    subgraph C["C++ 层"]
        MK["MiniKV<br/>LSM-Tree"]
        SN["SkyNet<br/>C++20 Coroutines"]
        LD["LumenDB<br/>HNSW + mmap"]
    end
    subgraph Py["Python 层"]
        AG["AgenticDB<br/>LLM Agent"]
        EM["Embedding<br/>sentence-transformers"]
        MC["MCP Server<br/>协议集成"]
    end
    subgraph LLM["LLM 后端"]
        OA["OpenAI API"]
        OL["Ollama 本地"]
    end
    AG --> OA
    AG --> OL
    AG --> LD
    MC --> LD
    LD --> MK
    LD --> SN
```

## License

MIT
