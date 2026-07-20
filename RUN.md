# DeepVector Monorepo — Multi-Platform Run Guide
# 多平台运行教程

技术选型与“为何不用过时方案”见 [TECH.md](./TECH.md)。

> **零成本跑通：** 免费 API / Ollama / 本地嵌入的完整清单见  
> [deepvector/course/FREE_RESOURCES_zh.md](./deepvector/course/FREE_RESOURCES_zh.md)（[English](./deepvector/course/FREE_RESOURCES_en.md)）  
> 配置模板：[`deepvector/.env.example`](./deepvector/.env.example)

---

## 1. 环境要求

| 组件 | 版本 |
|------|------|
| CMake | ≥ 3.20 |
| C++ 编译器 | GCC 12+ / Clang 15+（需 C++17/C++20） |
| Ninja | 推荐 |
| Python | 3.11+ |
| （可选）Ollama | 本地 LLM |
| （可选）Docker | 免本地编译 |

默认嵌入模型 `all-MiniLM-L6-v2` 输出 **384 维**。启动 C++ 服务时务必：

```bash
./deepvector_server --port 8080 --dim 384
```

---

## 2. Linux / macOS（推荐）

### 2.1 依赖

```bash
# Ubuntu / Debian
sudo apt-get update
sudo apt-get install -y g++-12 cmake ninja-build python3.11 python3.11-venv git

# macOS (Homebrew)
brew install cmake ninja python@3.11
```

### 2.2 编译

```bash
git clone <your-repo-url> hellocpp
cd hellocpp

cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_TESTS=ON \
  -DCMAKE_CXX_COMPILER=g++-12   # macOS 可省略或用 clang++

cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
ctest --test-dir build --output-on-failure
```

产物大致位置：

| 目标 | 路径 |
|------|------|
| DeepVector HTTP 服务 | `build/deepvector/deepvector_server` |
| 单元测试 | `build/deepvector/tests/deepvector_tests` |

单独编译子项目：

```bash
cmake -S minikv -B build-minikv -G Ninja -DENABLE_TESTS=ON && cmake --build build-minikv
cmake -S skynet -B build-skynet -G Ninja -DENABLE_TESTS=ON && cmake --build build-skynet
```

### 2.3 Python Agent

```bash
cd deepvector
python3.11 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

# 可选：本地 LLM
# curl -fsSL https://ollama.com/install.sh | sh
# ollama pull qwen2.5:7b
```

### 2.4 启动（两个终端）

```bash
# Terminal A — 向量库（维度必须与 embedding 一致）
./build/deepvector/deepvector_server --port 8080 --dim 384 --data-dir ./deepvector_data

# Terminal B — Agent
cd deepvector && source .venv/bin/activate
export AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
python -m agent.server.app
```

### 2.5 写入示例数据并查询

```bash
cd deepvector && source .venv/bin/activate
python scripts/demo_data.py

curl -s http://127.0.0.1:8080/health
curl -s http://127.0.0.1:8090/health

curl -s -X POST http://127.0.0.1:8090/ask \
  -H 'Content-Type: application/json' \
  -d '{"question":"什么是 RAG？"}'
```

直接搜向量库（需自备 384 维向量，或用 Agent/MCP 自动嵌入）：

```bash
# 带元数据插入
curl -s -X POST http://127.0.0.1:8080/insert \
  -H 'Content-Type: application/json' \
  -d '{"vector":[0.1,0.2,...], "meta":{"text":"hello","tags":"demo","timestamp":0}}'

# 取元数据
curl -s http://127.0.0.1:8080/vectors/0/meta
```

---

## 3. Windows

### 方案 A：WSL2（推荐）

1. 安装 [WSL2 + Ubuntu 22.04](https://learn.microsoft.com/windows/wsl/install)
2. 在 WSL 内按上面 **Linux** 步骤操作（仓库可放在 `\\wsl$\...` 或 `/mnt/c/...`）
3. 从 Windows 浏览器访问 `http://localhost:8080` / `8090`（WSL 端口默认转发）

### 方案 B：Docker Desktop

见下一节。原生 MSVC 目前不支持：`server.cpp` / MiniKV 使用 `sys/socket.h`、`unistd.h` 等 POSIX API。

### 方案 C：仅跑 Python Agent（连远程 Linux 上的 DeepVector）

```powershell
cd deepvector
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
$env:AGENTICDB_DEEPVECTOR_URL = "http://<linux-host>:8080"
python -m agent.server.app
```

---

## 4. Docker

在仓库根目录：

```bash
docker compose build
docker compose up
```

分别只构建某一阶段：

```bash
docker build -f deepvector/Dockerfile --target deepvector-runtime -t deepvector-db .
docker build -f deepvector/Dockerfile --target agent-runtime -t deepvector-agent .
```

---

## 5. 常用环境变量

| 变量 | 含义 | 默认 |
|------|------|------|
| `AGENTICDB_DEEPVECTOR_URL` | C++ 服务地址 | `http://localhost:8080` |
| `AGENTICDB_LLM_PROVIDER` | `ollama` / `openai` | `ollama` |
| `AGENTICDB_LLM_MODEL` | 模型名 | `qwen2.5:7b` |
| `AGENTICDB_OLLAMA_HOST` | Ollama 地址 | `http://localhost:11434` |
| `AGENTICDB_EMBEDDING_PROVIDER` | `local` / `openai` | `local` |
| `OPENAI_API_KEY` | OpenAI 兼容 Key | （空） |
| `OPENAI_BASE_URL` | 兼容 API 端点（ModelScope/硅基流动/Groq 等） | （空） |

**免费 LLM / 嵌入组合与注册链接：** [course/FREE_RESOURCES_zh.md](./deepvector/course/FREE_RESOURCES_zh.md) · 模板 [`deepvector/.env.example`](./deepvector/.env.example)

---

## 6. HTTP API 速查（DeepVector C++ :8080）

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/health` | 健康检查 |
| GET | `/stats` | 请求统计 |
| POST | `/collections` | 创建集合 `{"name":"foo"}` |
| DELETE | `/collections/:name` | 删除非 default 集合 |
| POST | `/search` | `{"vector","k","collection"?,"filter"?}` |
| POST | `/insert` | `vector`+`meta` 或 `vectors`+`metadatas` + `collection`? |
| GET | `/vectors/:id` | 取向量（及元数据） |
| GET | `/vectors/:id/meta` | 仅元数据 |
| GET | `/vector?id=` | 兼容旧路径 |
| DELETE | `/vectors/:id` | 删除 |

**注意：** 嵌入在 Python Agent / MCP / `demo_data.py` 侧完成；C++ 服务不提供 `/embed`。

Agent（:8090）：`GET /health`，`POST /query`，`POST /ask`，`POST /plan`。

---

## 7. 验证清单

```bash
# C++ 测试
ctest --test-dir build --output-on-failure

# Python Agent 单元测试
cd deepvector
source .venv/bin/activate   # Windows: .\.venv\Scripts\Activate.ps1
pip install pytest pytest-asyncio
pytest tests/agent -q

# 导入检查
python -c "from agent.server import create_app; print(create_app().title)"
```

---

## 8. 常见问题

1. **搜索报维度不匹配 / 结果乱**  
   服务 `--dim` 必须等于 embedding 维度（本地默认 384）。

2. **过滤搜不到**  
   插入时必须带 `meta`/`metadatas`（含 `tags`/`text`），否则 DocumentStore 无数据。

3. **Agent 答案空洞**  
   先跑 `scripts/demo_data.py`，确认 `GET /vectors/0/meta` 有 `text`。

4. **Windows 编译失败**  
   请用 WSL2 或 Docker，不要用原生 MSVC 直接编 `deepvector_server`。

5. **Ollama 连接失败**  
   确认 `ollama serve` 已启动，或改用 `AGENTICDB_LLM_PROVIDER=openai`。
