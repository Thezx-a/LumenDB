# INTERVIEW_BANK · 真实面试题库（项目映射）

> **真实性原则：** 下列题目均为向量检索 / 存储引擎 / 系统设计中的**高频考点方向**。  
> 来源类型：系统设计博客、MongoDB/Faiss 文档概念、LevelDB/RocksDB 经典问题、牛客/LeetCode「设计类」题型迁移。  
> **不是**从某一场保密面试逐字抄录；而是「企业真实会问的知识点」+ 结合本仓库可落地的答法。

中文讲解在前，English bullet answer 在后。做完对应章节再练。

---

## A. 向量与距离（Track A2 / P05）

### Q-A1 Cosine vs L2 vs Inner Product — when to use which?
**来源方向：** Vector DB interview guides（如 embeddings/similarity 专栏）、RAG 工程实践。

**中文要点：**
- L2：欧氏距离，未归一化通用；排序可用 **squared L2**（单调，省 `sqrt`）
- IP：越大越相似，常取负号纳入「最小化」框架
- Cosine：关注方向；向量已 L2 归一化时与 IP 等价

**English:** Prefer cosine/IP for text embeddings; L2 for raw geometric data. Use squared L2 for ranking.

**项目落地：** `include/dv/index/distance.h`

### Q-A2 Why is high-dimensional nearest neighbor hard?
**来源方向：** 维数灾难（curse of dimensionality）经典问答。

**要点：** 距离集中、指数级体积 → 精确搜索 O(N·d) 不可扩展 → 需要 ANN（HNSW/IVF/LSH）。

---

## B. HNSW（Track A3）— 超高频

### Q-B1 Explain HNSW like to an interviewer
**来源方向：** [MongoDB HNSW overview](https://www.mongodb.com/resources/basics/hierarchical-navigable-small-world)、Malkov & Yashunin 2018 论文概念、多家 Vector DB 面试指南。

**中文答法结构：**
1. 多层邻近图；上层稀疏「高速公路」，Layer0 稠密精修  
2. 插入：随机层 `floor(-ln(U)·1/ln(M))`，双向边，裁剪到 M / 2M  
3. 搜索：顶层贪心 → 逐层下降 → Layer0 用 ef 宽度 beam  
4. 权衡：M、efConstruction、efSearch ↔ 召回/延迟/内存

**English:** Hierarchical NSW graph; logarithmic navigation; tune M & ef*.

**项目：** `include/dv/index/hnsw.h`, `src/index/hnsw.cpp`

### Q-B2 Trade-offs of HNSW vs IVF-PQ
**来源方向：** 系统设计「Design a vector search engine」类题目。

| | HNSW | IVF-PQ |
|--|------|--------|
| 延迟 | 极低（内存图） | 中等 |
| 内存 | 高 | 低（压缩） |
| 更新 | 增量友好 | 训练/重建成本 |

### Q-B3 What does `efSearch` control?
越大：候选集越大 → 召回↑、延迟↑。本项目 `CollectionConfig.hnsw_ef_search`。

---

## C. mmap 与零拷贝（Track A4）

### Q-C1 mmap vs read/write
**来源方向：** Linux I/O、数据库内核面试。

**要点：** mmap 把文件映射进地址空间，按需缺页；适合随机读大文件。write+buffer 适合追加日志。DeepVector 向量用 mmap；WAL 用常规写。

### Q-C2 What does `msync(MS_SYNC)` guarantee?
把脏页刷到存储设备（等同强持久语义的一类 fsync 行为）。代价：延迟。

**项目：** `src/storage/vector_store.cpp`

---

## D. LSM-Tree / MiniKV（Track A5）— 存储岗高频

### Q-D1 Describe a write path in LSM
**来源方向：** LevelDB/RocksDB 面试八股（写路径几乎必问）。

**答：** Write → WAL → MemTable → 满则 freeze → flush SST L0 → compaction 下沉。

### Q-D2 What is write amplification?
一次用户写入触发多次磁盘写入（flush+compaction）。缓解：更大 MemTable、分层策略、布隆过滤减少读放大副作用。

### Q-D3 Why MergingIterator?
**来源方向：** LevelDB `MergingIterator` 设计。

**答：** 统一遍历 MemTable + Immutable + 各层 SST；堆合并；同 key 新版本优先。

**项目：** `minikv/src/core/merging_iterator.*`

### Q-D4 Bloom filter role on read
快速判断「一定不存在」→ 跳过 SST。假阳性率可调。

---

## E. 过滤与量化（Track A6–A7）

### Q-E1 Pre-filter vs post-filter
**要点：** DeepVector 当前是 **post-filter**（先 HNSW 再元数据）。高选择性过滤会迫使增大候选。

### Q-E2 Explain Product Quantization
子空间划分 + 每子空间码本；ADC 用距离表加速。Faiss 经典组件；面试常问压缩率与召回权衡。

---

## F. C++ / 并发 / 绑定（Track A8–A9）

### Q-F1 PIMPL benefits
编译防火墙、ABI 稳定、头文件依赖隔离。`DeepVectorServer::Impl`。

### Q-F2 Why release GIL in pybind11?
让其他 Python 线程运行；CPU 密集 C++ 搜索不应占着 GIL。

### Q-F3 `shared_mutex` for HNSW
多读者搜索 / 单写者插入。注意写时不能持有过期 mmap 指针跨 grow。

---

## G. HTTP / 可观测（Track A10/A12）

### Q-G1 select vs epoll
**来源方向：** 网络编程面试。

select：可移植、FD 集合有限；epoll：Linux 高并发。本教学服务器用 select；SkyNet/生产可 epoll。

### Q-G2 What metrics would you expose?
QPS、错误数、P99 延迟、集合数、连接数。本项目：`GET /metrics` Prometheus 文本。

---

## H. Agent / RAG / MCP（Track B）

### Q-H1 Draw a RAG pipeline
Embed query → ANN retrieve →（可选 rerank）→ prompt → LLM。DeepVector 覆盖 retrieve；Agent 覆盖 plan/evaluate。

### Q-H2 When should a multi-round agent stop?
质量分 ≥ 阈值，或达 max_rounds，或无新文档。对应 `ResultEvaluator.should_continue`。

### Q-H3 What is MCP?
**来源：** Anthropic Model Context Protocol 公开规范。统一工具发现与调用，使 IDE/Agent 可插拔连接 DeepVector 工具。

### Q-H4 FastAPI lifespan vs on_event
Pydantic v2 / FastAPI 现行推荐 lifespan；`on_event` 已弃用。本项目已迁移。

---

## I. 系统设计综合

### Q-I1 Design a vector database for 100M embeddings
**来源方向：** System design: vector search engine。

答题框架：
1. API：insert/search/filter  
2. Index：HNSW 分片 / IVF-PQ 冷热  
3. Storage：对象存储存原向量 + 内存/SSD 索引  
4. Consistency、多租户 collection、观测  

对照本项目：单机教学版 → 说明如何演进到分片。

### Q-I2 DeepVector vs Faiss vs Milvus（一句话）
Faiss：库；Milvus：分布式系统；DeepVector：可教学的嵌入式引擎+Agent 层。

---

## 刷题建议（像搭积木）

| 阶段 | 做哪些 |
|------|--------|
| 学完 A2–A3 | Q-A* Q-B* |
| 学完 A4–A5 | Q-C* Q-D* |
| 学完 A10 | Q-G* |
| 学完 B7 | Q-H* |
| Capstone 前 | Q-I1 白板 45 分钟 |

配套长问答：[`../INTERVIEW_QA.md`](../INTERVIEW_QA.md)

---

## References（题库出处类型，均可公开访问）

1. Malkov & Yashunin — *Efficient and robust approximate nearest neighbor search using HNSW graphs* (IEEE TPAMI / arXiv)  
2. MongoDB — Hierarchical Navigable Small World overview  
3. Meta Faiss documentation — indexes (HNSW, IVF, PQ)  
4. LevelDB / RocksDB architecture talks & docs — LSM write path  
5. Anthropic — Model Context Protocol specification  
6. FastAPI docs — Lifespan events  
7. Vector DB interview guides (public blogs summarizing embeddings + HNSW trade-offs)  
8. Classic OS texts / man pages — `mmap(2)`, `msync(2)`, `select(2)`
