<p align="center">
  <img src="https://img.shields.io/badge/Language-🇨🇳_中文-blue?style=for-the-badge" alt="中文"/>
  <img src="https://img.shields.io/badge/Language-🇺🇸_English-green?style=for-the-badge" alt="English"/>
  <img src="https://img.shields.io/badge/Pedagogy-点→线→面-orange?style=for-the-badge" alt="Pedagogy"/>
  <img src="https://img.shields.io/badge/Style-Hello--Agents_+_Datawhale-purple?style=for-the-badge" alt="Style"/>
</p>

<h1 align="center">DeepVector 从零到一 · Learning Path</h1>

<p align="center">
  <b>搭积木式学习：每一个「点」都可运行，串成「线」后拼成「面」</b><br/>
  <i>Build like LEGO: each brick runs alone, then wires into a production RAG stack</i>
</p>

<p align="center">
  <a href="./00_如何使用本教程_zh.md">🇨🇳 如何使用</a> ·
  <a href="./00_如何使用本教程_en.md">🇺🇸 How to use</a> ·
  <a href="./FREE_RESOURCES_zh.md">🆓 免费资源 Free APIs</a> ·
  <a href="./LEARNING_PATH.md">路线图 Roadmap</a> ·
  <a href="./INTERVIEW_BANK.md">面试题库 Interview Bank</a> ·
  <a href="../ARCHITECTURE.md">架构 Architecture</a> ·
  <a href="../../TECH.md">技术选型 TECH</a>
</p>

---

## 设计哲学（对齐 Hello-Agents / Datawhale）

| 原则 | 含义 |
|------|------|
| **造轮子 + 用轮子** | C++ 手写引擎；Python Agent 调用真实 LLM/嵌入生态 |
| **点 → 线 → 面** | 单知识点 → 模块串联 → 完整系统 |
| **每章可运行** | 有代码、有断言、有动手题 |
| **真实面试** | 题库来自向量库/存储引擎/系统设计常见考点（非臆造） |
| **双语** | 中文主讲 + English companion |

参考社区优秀实践：
- [datawhalechina/hello-agents](https://github.com/datawhalechina/hello-agents) — 从零构建 Agent、分篇递进、毕业设计
- Datawhale 开源学习社区 — 统一章节结构、练习与思考题

---

## 双轨课程地图（避免章节号冲突）

本仓库历史上存在 **C++ 引擎轨** 与 **Agent 智能检索轨** 两套章节目录。学习时请按轨道阅读，不要混用同号章节。

```mermaid
flowchart TB
    subgraph P0["Part 0 点 · Prerequisites"]
        PR[prerequisites/]
    end

    subgraph T1["Track A · C++ Engine 线"]
        A1[ch01_setup]
        A2[ch02_vectors_distance]
        A3[ch03_hnsw_theory]
        A4[ch04_mmap_storage]
        A5[ch05_lsm_tree]
        A6[ch06_metadata_filter]
        A7[ch07_quantization]
        A8[ch08_cpp_patterns]
        A9[ch09_python_bindings]
        A10[ch10_http_server]
        A11[ch11_coroutines]
        A12[ch12_production]
    end

    subgraph T2["Track B · AgenticDB 线"]
        B1[ch01_overview]
        B2[ch02_setup]
        B3[ch03_config]
        B4[ch04_llm]
        B5[ch05_embedding]
        B6[ch06_strategy]
        B7[ch07_multi_round]
        B8[ch08_mcp]
        B9[ch09_server]
        B10[ch10_cpp_enhance]
        B11[ch11_docker]
        B12[ch12_testing]
    end

    subgraph FACE["Part Capstone 面"]
        C13[ch13_capstone]
    end

    P0 --> T1
    P0 --> T2
    T1 --> FACE
    T2 --> FACE
```

### Track A — C++ 向量引擎（造发动机）

| 积木 | 目录 | 你会得到 |
|------|------|----------|
| A1 环境 | [ch01_setup](ch01_setup/) | CMake/Ninja 跑通 hello |
| A2 距离 | [ch02_vectors_distance](ch02_vectors_distance/) | L2/IP/Cosine + AVX2 |
| A3 HNSW | [ch03_hnsw_theory](ch03_hnsw_theory/) | 图索引搜索 |
| A4 mmap | [ch04_mmap_storage](ch04_mmap_storage/) | 零拷贝持久化 |
| A5 LSM | [ch05_lsm_tree](ch05_lsm_tree/) | MiniKV WAL→SST→Merge |
| A6 过滤 | [ch06_metadata_filter](ch06_metadata_filter/) | Filter AST |
| A7 量化 | [ch07_quantization](ch07_quantization/) | PQ/SQ |
| A8 模式 | [ch08_cpp_patterns](ch08_cpp_patterns/) | PIMPL / 类型擦除 |
| A9 绑定 | [ch09_python_bindings](ch09_python_bindings/) | pybind11 |
| A10 HTTP | [ch10_http_server](ch10_http_server/) | REST + multi-collection |
| A11 协程 | [ch11_coroutines](ch11_coroutines/) | C++20 Task（SkyNet） |
| A12 生产 | [ch12_production](ch12_production/) | Docker / metrics |

### Track B — AgenticDB（造智能检索）

| 积木 | 目录 | 你会得到 |
|------|------|----------|
| B1 概览 | [ch01_overview](ch01_overview/) | 全局架构图 |
| B2 环境 | [ch02_setup](ch02_setup/) | venv + Ollama |
| B3 配置 | [ch03_config](ch03_config/) | dataclass + env |
| B4 LLM | [ch04_llm](ch04_llm/) | Router / tools |
| B5 嵌入 | [ch05_embedding](ch05_embedding/) | local / OpenAI |
| B6 策略 | [ch06_strategy](ch06_strategy/) | DIRECT/FILTERED/… |
| B7 多轮 | [ch07_multi_round](ch07_multi_round/) | Plan→Eval→Reform |
| B8 MCP | [ch08_mcp](ch08_mcp/) | 工具协议 |
| B9 Agent HTTP | [ch09_server](ch09_server/) | FastAPI lifespan |
| B10 增强 | [ch10_cpp_enhance](ch10_cpp_enhance/) | 与引擎对接 |
| B11 Docker | [ch11_docker](ch11_docker/) | compose 双服务 |
| B12 测试 | [ch12_testing](ch12_testing/) | pytest agent |

### Capstone — 面

| | |
|--|--|
| [ch13_capstone](ch13_capstone/) | 端到端：编译 → 灌数 → Agent 问答 → 指标 |

---

## 三条学习路线（像 Hello-Agents 一样选档）

```mermaid
graph LR
    S((Start)) --> R{Route}
    R -->|🟢 周末入门 12h| G[A1-A3 + B1 + Capstone 精简]
    R -->|🟡 标准工程师 40h| Y[Track A 全 + Track B B1-B9]
    R -->|🔴 面试冲刺 70h| D[双轨全 + INTERVIEW_BANK + Capstone]
```

| 路线 | 适合谁 | 交付物 |
|------|--------|--------|
| 🟢 入门 | 会一点 C++/Python | 能搜索的小 demo |
| 🟡 标准 | 后端/检索方向 | 可 Docker 部署的双服务 |
| 🔴 面试 | 校招/社招存储&搜索 | 讲清 HNSW/LSM/mmap + Agent |

---

## 每章统一结构（强制模板）

详见 [`_CHAPTER_TEMPLATE.md`](_CHAPTER_TEMPLATE.md)。每章必须包含：

1. **点 Point** — 单一知识点 + 语法精讲  
2. **线 Line** — 与相邻模块如何对接  
3. **面 Surface** — 在整系统中的位置（架构图片段）  
4. **动手实践** — 可提交的小练习  
5. **反思思考** — 理解而非背诵  
6. **真实面试题** — 映射到 [INTERVIEW_BANK.md](INTERVIEW_BANK.md)  
7. **参考文档** — 论文 / 官方文档 / 源码路径（真实链接）

---

## 前置知识 Prerequisites

| 文档 | 中文 | English |
|------|------|---------|
| 构建环境 | [01](prerequisites/01_构建环境配置_zh.md) | [01](prerequisites/01_构建环境配置_en.md) |
| Docker | [02](prerequisites/02_Docker容器化_zh.md) | [02](prerequisites/02_Docker容器化_en.md) |
| Python | [03](prerequisites/03_Python环境_zh.md) | [03](prerequisites/03_Python环境_en.md) |
| **免费 API / Ollama** | [FREE_RESOURCES_zh](FREE_RESOURCES_zh.md) | [FREE_RESOURCES_en](FREE_RESOURCES_en.md) |
| 测试 | [04](prerequisites/04_测试框架_zh.md) | [04](prerequisites/04_测试框架_en.md) |
| 距离度量 | [05](prerequisites/05_向量距离度量_zh.md) | [05](prerequisites/05_向量距离度量_en.md) |
| SIMD | [06](prerequisites/06_SIMD与硬件优化_zh.md) | [06](prerequisites/06_SIMD与硬件优化_en.md) |

> 若存在历史重复文件（如 `05_距离度量_*`），以带「向量」字样的文件为准。

---

## 5 分钟冒烟（确认环境）

```bash
# from monorepo root (Linux / WSL / macOS)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON
cmake --build build -j$(nproc)
./build/deepvector/deepvector_server --port 8080 --dim 384 &
cd deepvector && pip install -r requirements.txt
python -c "from agent.server import create_app; print(create_app().title)"
curl -s http://127.0.0.1:8080/health
curl -s http://127.0.0.1:8080/metrics | head
```

Windows 请阅读根目录 [RUN.md](../../RUN.md)（WSL2 / Docker）。

---

## License

MIT · 欢迎像 Datawhale 一样提 PR 共创章节与题库。
