<p align="center">
  <img src="https://img.shields.io/badge/Language-🇨🇳_中文-blue?style=for-the-badge" alt="中文"/>
  <img src="https://img.shields.io/badge/Language-🇺🇸_English-green?style=for-the-badge" alt="English"/>
  <img src="https://img.shields.io/badge/Chapters-13-blueviolet?style=for-the-badge" alt="Chapters"/>
  <img src="https://img.shields.io/badge/Exercises-40+-green?style=for-the-badge" alt="Exercises"/>
  <img src="https://img.shields.io/badge/Thinking_Questions-80+-orange?style=for-the-badge" alt="Questions"/>
  <img src="https://img.shields.io/badge/License-MIT-blue?style=for-the-badge" alt="MIT"/>
</p>

<h1 align="center">DeepVector 从零到一</h1>

<p align="center">
  <b>C++ 向量数据库实战教程</b><br/>
  <i>从第一行代码到生产部署 · 13 章 · 70+ 小时内容</i>
</p>

---

## 前置知识（Prerequisites）

各章共享的基础知识已提取到独立文档，避免重复：

| 文档 | 内容 | 中文 | English |
|------|------|------|---------|
| 构建环境配置 | CMake, Ninja, g++, 编译选项 | [中文](prerequisites/01_构建环境配置_zh.md) | [English](prerequisites/01_构建环境配置_en.md) |
| Docker 容器化 | Dockerfile, docker-compose, 镜像优化 | [中文](prerequisites/02_Docker容器化_zh.md) | [English](prerequisites/02_Docker容器化_en.md) |
| Python 环境 | 虚拟环境, pip, pybind11 构建 | [中文](prerequisites/03_Python环境_zh.md) | [English](prerequisites/03_Python环境_en.md) |
| 测试框架 | Google Test, ctest, 断言宏 | [中文](prerequisites/04_测试框架_zh.md) | [English](prerequisites/04_测试框架_en.md) |
| 向量距离度量 | L2, Cosine, Inner Product | [中文](prerequisites/05_向量距离度量_zh.md) | [English](prerequisites/05_向量距离度量_en.md) |
| SIMD 与硬件优化 | AVX2, 内存层次, 编译选项 | [中文](prerequisites/06_SIMD与硬件优化_zh.md) | [English](prerequisites/06_SIMD与硬件优化_en.md) |

> 每章开头的 **前置知识** 区域会标注需要先阅读哪些参考文档。

---

## 为什么学这个？

RAG (Retrieval-Augmented Generation) 是让大语言模型"查资料再回答"的核心技术。而 RAG 的底层就是一个**向量数据库**。

本教程带你从零开始，用 C++ 手写一个完整的向量数据库引擎——涵盖 HNSW 搜索、SIMD 加速、mmap 存储、量化压缩、Python 绑定、HTTP 服务器、生产部署。

**不是用框架搭积木，是造发动机。**

```mermaid
mindmap
  root((DeepVector 教程))
    存储层
      mmap 零拷贝
      LSM-Tree
      WAL 持久化
    搜索层
      HNSW 图索引
      SIMD 加速
      向量距离
    压缩层
      PQ 量化
      SQ 量化
      k-means
    应用层
      Python 绑定
      HTTP 服务器
      元数据过滤
    生产层
      Docker
      CI/CD
      监控
```

---

## 课程大纲

```mermaid
graph LR
    subgraph "基础 (Ch1-3)"
        C01[Ch01 环境搭建]
        C02[Ch02 向量距离]
        C03[Ch03 HNSW]
    end

    subgraph "存储 (Ch4-5)"
        C04[Ch04 mmap]
        C05[Ch05 LSM-Tree]
    end

    subgraph "功能 (Ch6-9)"
        C06[Ch06 过滤]
        C07[Ch07 量化]
        C08[Ch08 C++模式]
        C09[Ch09 Python]
    end

    subgraph "服务 (Ch10-12)"
        C10[Ch10 HTTP]
        C11[Ch11 协程]
        C12[Ch12 生产]
    end

    subgraph "实战"
        C13[Ch13 终极项目]
    end

    C01 --> C02 --> C03
    C03 --> C04 --> C05
    C05 --> C06 --> C07 --> C08 --> C09
    C09 --> C10 --> C11 --> C12
    C12 --> C13

    style C13 fill:#fff3e0
    style C03 fill:#e1f5fe
    style C05 fill:#f3e5f5
```

| 章 | 中文 | English | 难度 | 时间 |
|----|------|---------|------|------|
| 01 | [环境搭建](ch01_setup/01_环境搭建与编译_zh.md) | [Setup](ch01_setup/01_环境搭建与编译_en.md) | ⭐ | 2h |
| 02 | [向量与距离](ch02_vectors_distance/02_向量与距离度量_zh.md) | [Vectors & Distance](ch02_vectors_distance/02_向量与距离度量_en.md) | ⭐⭐ | 3h |
| 03 | [HNSW 算法](ch03_hnsw_theory/03_HNSW近似搜索_zh.md) | [HNSW Search](ch03_hnsw_theory/03_HNSW近似搜索_en.md) | ⭐⭐⭐ | 4h |
| 04 | [mmap 存储](ch04_mmap_storage/04_mmap零拷贝存储_zh.md) | [mmap Storage](ch04_mmap_storage/04_mmap零拷贝存储_en.md) | ⭐⭐ | 3h |
| 05 | [LSM-Tree](ch05_lsm_tree/05_LSM-Tree存储引擎_zh.md) | [LSM-Tree Engine](ch05_lsm_tree/05_LSM-Tree存储引擎_en.md) | ⭐⭐⭐ | 5h |
| 06 | [元数据过滤](ch06_metadata_filter/06_元数据过滤搜索_zh.md) | [Metadata Filter](ch06_metadata_filter/06_元数据过滤搜索_en.md) | ⭐⭐ | 3h |
| 07 | [量化压缩](ch07_quantization/07_向量量化压缩_zh.md) | [Quantization](ch07_quantization/07_向量量化压缩_en.md) | ⭐⭐⭐ | 4h |
| 08 | [C++ 设计模式](ch08_cpp_patterns/08_CPP设计模式_zh.md) | [C++ Patterns](ch08_cpp_patterns/08_CPP设计模式_en.md) | ⭐⭐ | 3h |
| 09 | [Python 绑定](ch09_python_bindings/09_Python绑定_zh.md) | [Python Bindings](ch09_python_bindings/09_Python绑定_en.md) | ⭐⭐ | 3h |
| 10 | [HTTP 服务器](ch10_http_server/10_HTTP服务器设计_zh.md) | [HTTP Server](ch10_http_server/10_HTTP服务器设计_en.md) | ⭐⭐ | 4h |
| 11 | [C++20 协程](ch11_coroutines/11_CPP20协程_zh.md) | [C++20 Coroutines](ch11_coroutines/11_CPP20协程_en.md) | ⭐⭐⭐ | 4h |
| 12 | [生产部署](ch12_production/12_生产部署_zh.md) | [Production](ch12_production/12_生产部署_en.md) | ⭐⭐ | 3h |
| 13 | [终极项目](ch13_capstone/13_终极项目_zh.md) | [Capstone Project](ch13_capstone/13_终极项目_en.md) | ⭐⭐⭐ | 7天 |

---

## 学习路线

```mermaid
graph TB
    START((开始)) --> ROUTE{选择路线}
    
    ROUTE -->|🟢 入门| G1[Ch01] --> G2[Ch02] --> G3[Ch03] --> G13A[Ch13 终极项目]
    
    ROUTE -->|🟡 标准| S1[Ch01~Ch08] --> S2[Ch09] --> S3[Ch13 终极项目]
    
    ROUTE -->|🔴 面试| D1[全部 13 章] --> D2[每章思考题] --> D3[INTERVIEW_QA.md]
    
    G13A --> DONE((完成))
    S3 --> DONE
    D3 --> DONE

    style G13A fill:#c8e6c9
    style S3 fill:#fff9c4
    style D3 fill:#ffcdd2
    style DONE fill:#e8f5e9
```

| 路线 | 路径 | 适合 |
|------|------|------|
| 🟢 入门 | Ch1 → Ch2 → Ch3 → Ch13 | 想快速跑通全流程 |
| 🟡 标准 | Ch1 ~ Ch08 → Ch13 | 想全面掌握技术栈 |
| 🔴 面试 | 全部 + 思考题 + QA | 准备 C++ 面试 |

---

## 每章结构

```mermaid
graph TB
    CH[每章内容] --> THEORY[📖 理论知识<br/>原理 + 公式 + 类比]
    CH --> CODE[💻 代码练习<br/>3-5 个递进式任务]
    CH --> THINK[🧠 思考题<br/>5-8 道深度检验]
    CH --> CHECK[📋 知识点<br/>可勾选 checklist]

    style THEORY fill:#e1f5fe
    style CODE fill:#e8f5e9
    style THINK fill:#fff3e0
    style CHECK fill:#f3e5f5
```

---

## 技术栈

```mermaid
graph TB
    subgraph "Language"
        CPP17[C++17]
        CPP20[C++20]
        PY[Python]
    end

    subgraph "Build"
        CMAKE[CMake]
        NINJA[Ninja]
        GCC[g++-12]
    end

    subgraph "Libraries"
        GTEST[Google Test]
        PB11[pybind11]
        JSON[nlohmann/json]
    end

    subgraph "DevOps"
        DOCKER[Docker]
        GHA[GitHub Actions]
    end

    CPP17 --> CMAKE
    CPP20 --> CMAKE
    CMAKE --> NINJA
    CMAKE --> GCC
    CMAKE --> GTEST
    CMAKE --> PB11
    DOCKER --> GHA

    style CPP17 fill:#e1f5fe
    style CPP20 fill:#e8f5e9
```

| 层 | 技术 |
|----|------|
| 语言 | C++17 / C++20 / Python |
| 构建 | CMake + Ninja |
| 编译器 | g++-12 / clang++-14 |
| 测试 | Google Test |
| 绑定 | pybind11 |
| 服务器 | HTTP/1.1 + Bearer Auth |
| 部署 | Docker + docker-compose |
| CI/CD | GitHub Actions |

---

## 环境要求

| 需求 | 最低 | 推荐 |
|------|------|------|
| OS | Ubuntu 20.04 / WSL2 | Ubuntu 22.04 |
| 编译器 | g++-11 | g++-12 |
| 构建 | CMake 3.16+ | CMake 3.25+ |
| 内存 | 4GB | 8GB+ (Ch13) |
| 编辑器 | 任意 | VS Code + C/C++ |

---

## 配套仓库

```mermaid
graph LR
    COURSE[DeepVector 教程] --> LDB[DeepVector<br/>向量数据库]
    COURSE --> MKV[MiniKV<br/>存储引擎]
    COURSE --> SN[SkyNet<br/>网络框架]

    LDB --> MKV
    LDB --> SN

    style COURSE fill:#e1f5fe
    style LDB fill:#fff3e0
    style MKV fill:#f3e5f5
    style SN fill:#e8f5e9
```

| 仓库 | 说明 |
|------|------|
| [DeepVector](https://github.com/Thezx-a/DeepVector) | 向量数据库主仓库 |
| [MiniKV](https://github.com/Thezx-a/MiniKV) | LSM-Tree 存储引擎 |
| [SkyNet](https://github.com/Thezx-a/SkyNet) | C++20 协程网络框架 |

---

## 参考资源

- [HNSW 论文 (Malkov & Yashunin, 2016)](https://arxiv.org/abs/1603.09320)
- [FAISS 源码](https://github.com/facebookresearch/faiss)
- [LevelDB 源码](https://github.com/google/leveldb)
- [RocksDB Wiki](https://github.com/facebook/rocksdb/wiki)
- [pybind11 文档](https://pybind11.readthedocs.io/)
- [DeepVector 面试 78 题](https://github.com/Thezx-a/DeepVector/blob/main/INTERVIEW_QA.md)

---

## License

[MIT](LICENSE) — 自由使用、修改、分发。

---

<p align="center">
  <i>造一个数据库，理解整个系统。</i>
</p>
