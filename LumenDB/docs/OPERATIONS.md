# AgenticDB 操作手册 / Operations Manual

## 目录 / Table of Contents

<!-- TOC -->
- [1. 系统概览 / System Overview](#1-系统概览--system-overview)
- [2. 环境要求 / Prerequisites](#2-环境要求--prerequisites)
- [3. 安装指南 / Installation](#3-安装指南--installation)
- [4. 配置说明 / Configuration](#4-配置说明--configuration)
- [5. 启动运行 / Running](#5-启动运行--running)
- [6. API 参考 / API Reference](#6-api-参考--api-reference)
- [7. 数据管理 / Data Management](#7-数据管理--data-management)
- [8. 故障排除 / Troubleshooting](#8-故障排除--troubleshooting)
- [9. 性能优化 / Performance Tuning](#9-性能优化--performance-tuning)
- [10. 生产部署检查清单 / Production Checklist](#10-生产部署检查清单--production-checklist)

---

## 1. 系统概览 / System Overview

### 架构图 / Architecture

```
┌──────────────────────────────────────────────────────────┐
│                       用户 / User                         │
│              "帮我找 RAG 相关的论文"                        │
└────────────────────────┬─────────────────────────────────┘
                         │ HTTP (port 8090)
                         ▼
┌──────────────────────────────────────────────────────────┐
│              AgenticDB Agent Server (Python)              │
│                                                          │
│  ┌─────────────────┐  ┌──────────────────────────────┐   │
│  │  HTTP API        │  │  MultiRoundEngine            │   │
│  │  /query  /ask    │  │  ┌────────┐ ┌──────────┐   │   │
│  │  /plan  /health  │  │  │Planner │ │Evaluator  │   │   │
│  └─────────────────┘  │  ├────────┤ ├──────────┤   │   │
│                       │  │Reformer│ │Generator  │   │   │
│                       │  └────────┘ └──────────┘   │   │
│                       └──────────────────────────────┘   │
│                                                          │
│  ┌─────────────────┐  ┌──────────────────────────────┐   │
│  │  LLM Router      │  │  Embedding Service           │   │
│  │  OpenAI / Ollama │  │  Local / OpenAI              │   │
│  └─────────────────┘  └──────────────────────────────┘   │
│                                                          │
│  ┌─────────────────┐                                     │
│  │  MCP Server      │  ← Agent 框架集成 (可选)          │
│  └─────────────────┘                                     │
└────────────────────────┬─────────────────────────────────┘
                         │ HTTP (port 8080)
                         ▼
┌──────────────────────────────────────────────────────────┐
│              LumenDB C++ Server                          │
│  /search  /insert  /collections  /batch                  │
│  HNSW + mmap + MiniKV + PQ/SQ                           │
└──────────────────────────────────────────────────────────┘
```

### 组件职责 / Component Responsibilities

| 组件 | 语言 | 端口 | 职责 |
|------|------|------|------|
| LumenDB Server | C++17 | 8080 | 向量存储、索引、搜索、持久化 |
| Agent Server | Python 3.11+ | 8090 | LLM 交互、查询规划、多轮检索、MCP |
| Ollama (可选) | Go | 11434 | 本地 LLM 推理 |
| OpenAI API (可选) | 云端 | - | 云端 LLM + 嵌入 |

---

## 2. 环境要求 / Prerequisites

### 硬件要求 / Hardware Requirements

| 配置 | 最低 | 推荐 |
|------|------|------|
| CPU | 4 核 | 8 核+ (支持 AVX2) |
| RAM | 8 GB | 16 GB (本地 LLM 需要) |
| 磁盘 | 10 GB | 50 GB+ (SSD 推荐) |
| GPU (可选) | - | NVIDIA + CUDA (加速本地 LLM) |

### 软件要求 / Software Requirements

| 软件 | 版本 | 用途 |
|------|------|------|
| Python | 3.11+ | Agent 层 |
| CMake | 3.16+ | C++ 构建 |
| GCC | g++-12 (Linux/WSL2) | C++ 编译 |
| Ninja | 1.10+ (可选) | 加速 C++ 构建 |
| Ollama | 0.32+ (可选) | 本地 LLM |
| Docker | 24+ (可选) | 容器化部署 |

### Windows 特殊说明

Windows 开发需要 WSL2 (Ubuntu 22.04):

```powershell
# 1. 安装 WSL2
wsl --install -d Ubuntu-22.04

# 2. 在 WSL2 中安装依赖
wsl
sudo apt update
sudo apt install -y g++-12 cmake ninja-build
```

---

## 3. 安装指南 / Installation

### Step 1: 克隆仓库 / Clone Repository

```bash
git clone --recursive https://github.com/Thezx-a/LumenDB.git
cd LumenDB
```

如果已经克隆但子模块未初始化:

```bash
git submodule update --init --recursive
```

### Step 2: 安装 Python 依赖 / Install Python Dependencies

```bash
# 核心依赖 / Core dependencies
pip install httpx pydantic sentence-transformers

# ⚠️ 安装慢的解决方案 / If slow, use mirror:
pip install -i https://pypi.tuna.tsinghua.edu.cn/simple httpx pydantic sentence-transformers

# 可选: FastAPI 服务器 (推荐) / For FastAPI server (recommended)
pip install fastapi uvicorn

# 可选: MCP Server / For MCP protocol support
pip install mcp

# 可选: 测试框架 / For running tests
pip install pytest pytest-asyncio
```

### Step 3: 编译 C++ Server / Build C++ Server

**Linux / WSL2:**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-12
cmake --build build --target lumendb_server -j$(nproc)
```

**Windows (原生 MSVC) 暂不支持**, 请在 WSL2 中构建。

验证编译成功:

```bash
./build/server/lumendb_server --help
# 预期输出: 显示命令行参数选项
```

### Step 4: 安装 Ollama (可选) / Install Ollama (Optional)

```bash
# Linux / WSL2
curl -fsSL https://ollama.com/install.sh | sh

# 拉取嵌入模型 (必需) / Pull embedding model (required)
ollama pull nomic-embed-text

# 拉取 LLM 模型 / Pull LLM model
ollama pull qwen2.5:3b
# 或更大的模型 / Or larger model:
# ollama pull qwen2.5:7b

# 验证 / Verify
ollama list
# 应显示: nomic-embed-text 和 qwen2.5:3b
```

> ⚠️ **网络慢怎么办?** 使用代理或国内镜像:
> ```bash
> # 设置代理 / Set proxy
> export http_proxy=http://127.0.0.1:7890
> export https_proxy=http://127.0.0.1:7890
> ```

---

## 4. 配置说明 / Configuration

### 环境变量 / Environment Variables

所有配置可通过环境变量覆盖:

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `AGENTICDB_LLM_PROVIDER` | `ollama` | `openai` 或 `ollama` |
| `AGENTICDB_LLM_MODEL` | `qwen2.5:7b` | LLM 模型名称 |
| `AGENTICDB_EMBEDDING_PROVIDER` | `local` | `local` 或 `openai` |
| `AGENTICDB_EMBEDDING_MODEL` | `all-MiniLM-L6-v2` | 本地嵌入模型 |
| `OPENAI_API_KEY` | - | OpenAI API 密钥 |
| `AGENTICDB_LUMENDB_URL` | `http://localhost:8080` | LumenDB 地址 |
| `AGENTICDB_AGENT_PORT` | `8090` | Agent 服务端口 |
| `AGENTICDB_MAX_ROUNDS` | `5` | 最大检索轮数 |
| `AGENTICDB_QUALITY_THRESHOLD` | `0.7` | 质量评分阈值 |

### 配置文件 / Config File

也可以通过 Python 代码配置:

```python
from agent.config import AgenticDBConfig, LLMConfig, EmbeddingConfig

config = AgenticDBConfig(
    llm=LLMConfig(
        provider="openai",
        model="gpt-4o",
        openai_api_key="sk-xxx",
        temperature=0.1,
    ),
    embedding=EmbeddingConfig(
        provider="openai",
        openai_model="text-embedding-3-small",
    ),
    max_rounds=3,
    quality_threshold=0.8,
)
```

---

## 5. 启动运行 / Running

### 最小启动 (LumenDB + 本地嵌入, 无 LLM)

```bash
# 终端 1: 启动 LumenDB
cd LumenDB
./build/server/lumendb_server --port 8080 --dim 384 --data-dir ./data

# 验证
curl http://localhost:8080/health
# {"status":"ok","vectors":0,"dim":384}
```

### 完整启动 (LumenDB + Agent + Ollama)

```bash
# 终端 1: 启动 Ollama (如果使用本地 LLM)
ollama serve

# 终端 2: 启动 LumenDB
./build/server/lumendb_server --port 8080 --dim 384 --data-dir ./data

# 终端 3: 启动 Agent Server
cd LumenDB
python agent/server/app.py
# 启动 FastAPI 服务器: http://0.0.0.0:8090

# 终端 4: 灌入数据 + 运行 Demo
python scripts/demo_data.py
python examples/demo_agentic_search.py
```

### 使用 OpenAI (无需 Ollama)

```bash
# 终端 1: 启动 LumenDB
./build/server/lumendb_server --port 8080 --dim 1536 --data-dir ./data

# 终端 2: 启动 Agent Server (OpenAI 模式)
export AGENTICDB_LLM_PROVIDER=openai
export AGENTICDB_LLM_MODEL=gpt-4o
export AGENTICDB_EMBEDDING_PROVIDER=openai
export AGENTICDB_EMBEDDING_MODEL=text-embedding-3-small
export OPENAI_API_KEY=sk-your-key-here

python agent/server/app.py
```

### Docker 部署 / Docker Deployment

```bash
# 构建镜像 / Build image
docker build -t lumendb:latest .

# 启动容器 / Run container
docker run -d \
  --name lumendb \
  -p 8080:8080 \
  -v ./data:/data \
  -e LUMENDB_DIM=384 \
  lumendb:latest
```

---

## 6. API 参考 / API Reference

### LumenDB C++ Server (port 8080)

#### `GET /health`
健康检查 / Health check.

```bash
curl http://localhost:8080/health
# {"status":"ok","vectors":1024,"dim":384}
```

#### `POST /search`
向量搜索 / Vector search.

```bash
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"vector": [0.1, 0.2, ...], "k": 10}'
# {"results":[{"id":1,"distance":0.23},...]}
```

支持过滤 / With filter:

```bash
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"vector": [...], "k": 10, "filter": {"op":"eq","field":"tags","value":"RAG"}}'
```

#### `POST /insert`
插入向量 / Insert vector.

```bash
curl -X POST http://localhost:8080/insert \
  -H "Content-Type: application/json" \
  -d '{"vector": [0.1, 0.2, ...]}'
# {"ids":[1]}
```

批量插入 / Batch insert:

```bash
curl -X POST http://localhost:8080/insert \
  -H "Content-Type: application/json" \
  -d '{"vectors": [[0.1, ...], [0.2, ...]]}'
```

#### `GET /collections`
列出集合列表 / List collections.

```bash
curl http://localhost:8080/collections
# {"collections":[{"name":"default","vectors":1024,"dim":384}]}
```

#### `POST /batch/search`
批量搜索 / Batch search.

```bash
curl -X POST http://localhost:8080/batch/search \
  -H "Content-Type: application/json" \
  -d '{"queries": [{"vector": [...], "k": 5}, {"vector": [...], "k": 5}]}'
```

#### `DELETE /vectors/:id`
删除向量 / Delete vector.

```bash
curl -X DELETE http://localhost:8080/vectors/1
# {"status":"ok"}
```

### Agent Server (port 8090)

#### `GET /health`
```bash
curl http://localhost:8090/health
# {"status":"ok","model":"qwen2.5:7b","provider":"ollama"}
```

#### `POST /query`
完整 Agent 检索 / Full agent search.

```bash
curl -X POST http://localhost:8090/query \
  -H "Content-Type: application/json" \
  -d '{"question": "What is RAG?", "max_rounds": 3}'
# {
#   "answer": "RAG is Retrieval-Augmented Generation...",
#   "strategy": "direct",
#   "rounds": 1,
#   "quality_score": 0.85,
#   "queries_tried": ["RAG overview"]
# }
```

#### `POST /ask`
简洁问答 / Simple Q&A.

```bash
curl -X POST http://localhost:8090/ask \
  -H "Content-Type: application/json" \
  -d '{"question": "Explain HNSW"}'
# {"answer": "HNSW is Hierarchical Navigable Small World...", "rounds": 1}
```

#### `POST /plan`
仅查看检索计划 (不执行) / Preview plan only.

```bash
curl -X POST http://localhost:8090/plan \
  -H "Content-Type: application/json" \
  -d '{"question": "Compare HNSW and IVF"}'
# {"strategy": "multi_query", "reasoning": "Multi-part comparison", "steps": 2}
```

---

## 7. 数据管理 / Data Management

### 灌入示例数据 / Insert Demo Data

```bash
python scripts/demo_data.py
# 预期输出:
#   Inserting 15 documents into LumenDB...
#   [1/15] Inserted doc 1
#   [2/15] Inserted doc 2
#   ...
#   Done!
```

### 自定义数据集 / Custom Dataset

```python
from agent.embedding.service import EmbeddingService
import httpx
import numpy as np

# 准备数据 / Prepare data
documents = [
    "Document 1 text...",
    "Document 2 text...",
]

# 嵌入 / Embed
svc = EmbeddingService()
vectors = await svc.embed(documents)

# 插入 / Insert
async with httpx.AsyncClient() as client:
    for vec in vectors:
        resp = await client.post(
            "http://localhost:8080/insert",
            json={"vector": vec},
        )
        print(f"Inserted: {resp.json()['ids'][0]}")
```

### 数据持久化 / Data Persistence

LumenDB 使用 mmap 持久化到磁盘。数据目录结构:

```
./data/
├── vectors.bin        # 向量数据 (mmap)
├── docs/              # 元数据 (MiniKV LSM-Tree)
│   ├── MANIFEST-00001
│   ├── CURRENT
│   └── *.sst
└── *.cfg.json         # 集合配置
```

备份: 直接复制 data/ 目录即可。

---

## 8. 故障排除 / Troubleshooting

### 常见问题 / Common Issues

#### Q: LumenDB 启动失败 "failed to bind to port"

```bash
# 检查端口占用 / Check port usage
netstat -ano | grep 8080

# 使用不同端口 / Use different port
./build/server/lumendb_server --port 8081
```

#### Q: Python 导入 agent 包失败 / ImportError

```bash
# 确保在 LumenDB 目录下运行 / Run from LumenDB directory
cd LumenDB

# 检查 PYTHONPATH / Check Python path
$env:PYTHONPATH = "$env:PYTHONPATH;."
python -c "from agent.config import load_config; print(load_config())"
```

#### Q: Ollama 连接被拒绝 / Connection refused

```bash
# 检查 Ollama 是否运行 / Check if Ollama is running
ollama list

# 启动 Ollama 服务 / Start Ollama
ollama serve &

# 设置 Ollama 地址 / Set Ollama host
set AGENTICDB_OLLAMA_HOST=http://127.0.0.1:11434
```

#### Q: 检索结果为空的常见原因 / Empty Search Results

1. 嵌入模型维度不匹配: LumenDB `--dim` 必须与 embedding 输出维度一致
   - all-MiniLM-L6-v2: 384
   - text-embedding-3-small: 1536
   - nomic-embed-text: 768
2. 没有数据: 先运行 `python scripts/demo_data.py`
3. LumenDB 未启动: 检查 `curl http://localhost:8080/health`

#### Q: C++ 编译错误 / Build Errors

```bash
# 清理重试 / Clean rebuild
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target lumendb_server

# 检查编译器 / Check compiler
g++-12 --version
```

---

## 9. 性能优化 / Performance Tuning

### C++ Server 优化

| 参数 | 默认 | 说明 |
|------|------|------|
| `--dim` | 768 | 向量维度, 越小搜索越快 |
| HNSW `M` | 16 | 图最大邻居数, 越大召回率越高但内存和延迟增加 |
| HNSW `ef_search` | 50 | 搜索宽度, 越大召回率越高但延迟增加 |
| `USE_AVX2` | ON | 启用 SIMD 加速 (编译选项) |

### Agent Server 优化

| 参数 | 默认 | 优化建议 |
|------|------|---------|
| `max_rounds` | 5 | 简单查询设为 1-2, 复杂查询 3-5 |
| `quality_threshold` | 0.7 | 质量要求高设为 0.8, 速度快设为 0.5 |
| `temperature` | 0.1 | 需要确定性设为 0.0, 需要创意设为 0.7 |
| `top_k_final` | 10 | 最终返回结果数, 不影响质量但影响 token 消耗 |

### 批处理 / Batch Processing

```bash
# 批量插入 10000 个向量
python -c "
import httpx, numpy as np
vectors = np.random.randn(10000, 384).astype(np.float32)
for batch in np.array_split(vectors, 10):
    resp = httpx.post('http://localhost:8080/insert',
        json={'vectors': batch.tolist()})
    print(f'Inserted batch: {resp.json()}')
"
```

---

## 10. 生产部署检查清单 / Production Checklist

### 安全 / Security
- [ ] 设置 LumenDB `--api-key` 防止未授权访问
- [ ] 使用环境变量管理密钥, 不硬编码
- [ ] 将 Agent Server 置于反向代理后 (nginx/Caddy)
- [ ] 启用 HTTPS (Let's Encrypt)

### 可靠性 / Reliability
- [ ] 使用 systemd/supervisor 管理进程, 支持自动重启
- [ ] 配置数据定期备份
- [ ] 设置资源限制 (ulimit/RLIMIT)
- [ ] 监控: 接入 Prometheus + Grafana

### 性能 / Performance
- [ ] 使用 GPU 加速本地 LLM (Ollama + CUDA)
- [ ] 使用 SSD 存储向量数据
- [ ] 调整 HNSW 参数平衡召回率和延迟
- [ ] 启用响应缓存 (Redis) 减少重复查询

### 运维 / Operations
- [ ] 配置日志轮转 (logrotate)
- [ ] 设置健康检查端点和告警
- [ ] 准备容量规划指南
- [ ] 编写故障恢复 SOP

### 示例 systemd 服务 / Example systemd Service

```ini
# /etc/systemd/system/lumendb.service
[Unit]
Description=LumenDB Vector Database Server
After=network.target

[Service]
Type=simple
User=lumendb
WorkingDirectory=/opt/lumendb
ExecStart=/opt/lumendb/build/server/lumendb_server \
  --port 8080 --dim 384 --data-dir /opt/lumendb/data --api-key ${API_KEY}
Restart=always
RestartSec=5
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```
