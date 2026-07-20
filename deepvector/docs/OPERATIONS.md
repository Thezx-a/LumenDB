# AgenticDB 鎿嶄綔鎵嬪唽 / Operations Manual

## 鐩綍 / Table of Contents

<!-- TOC -->
- [1. 绯荤粺姒傝 / System Overview](#1-绯荤粺姒傝--system-overview)
- [2. 鐜瑕佹眰 / Prerequisites](#2-鐜瑕佹眰--prerequisites)
- [3. 瀹夎鎸囧崡 / Installation](#3-瀹夎鎸囧崡--installation)
- [4. 閰嶇疆璇存槑 / Configuration](#4-閰嶇疆璇存槑--configuration)
- [5. 鍚姩杩愯 / Running](#5-鍚姩杩愯--running)
- [6. API 鍙傝€?/ API Reference](#6-api-鍙傝€?-api-reference)
- [7. 鏁版嵁绠＄悊 / Data Management](#7-鏁版嵁绠＄悊--data-management)
- [8. 鏁呴殰鎺掗櫎 / Troubleshooting](#8-鏁呴殰鎺掗櫎--troubleshooting)
- [9. 鎬ц兘浼樺寲 / Performance Tuning](#9-鎬ц兘浼樺寲--performance-tuning)
- [10. 鐢熶骇閮ㄧ讲妫€鏌ユ竻鍗?/ Production Checklist](#10-鐢熶骇閮ㄧ讲妫€鏌ユ竻鍗?-production-checklist)

---

## 1. 绯荤粺姒傝 / System Overview

### 鏋舵瀯鍥?/ Architecture

```
鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?                      鐢ㄦ埛 / User                         鈹?鈹?             "甯垜鎵?RAG 鐩稿叧鐨勮鏂?                        鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                         鈹?HTTP (port 8090)
                         鈻?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?             AgenticDB Agent Server (Python)              鈹?鈹?                                                         鈹?鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?鈹? 鈹? HTTP API        鈹? 鈹? MultiRoundEngine            鈹?  鈹?鈹? 鈹? /query  /ask    鈹? 鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?  鈹?鈹? 鈹? /plan  /health  鈹? 鈹? 鈹侾lanner 鈹?鈹侲valuator  鈹?  鈹?  鈹?鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹? 鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?  鈹?鈹?                      鈹? 鈹俁eformer鈹?鈹侴enerator  鈹?  鈹?  鈹?鈹?                      鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?  鈹?鈹?                      鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?鈹?                                                         鈹?鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?鈹? 鈹? LLM Router      鈹? 鈹? Embedding Service           鈹?  鈹?鈹? 鈹? OpenAI / Ollama 鈹? 鈹? Local / OpenAI              鈹?  鈹?鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?  鈹?鈹?                                                         鈹?鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                                    鈹?鈹? 鈹? MCP Server      鈹? 鈫?Agent 妗嗘灦闆嗘垚 (鍙€?          鈹?鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                                    鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?                         鈹?HTTP (port 8080)
                         鈻?鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹?             DeepVector C++ Server                          鈹?鈹? /search  /insert  /collections  /batch                  鈹?鈹? HNSW + mmap + MiniKV + PQ/SQ                           鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

### 缁勪欢鑱岃矗 / Component Responsibilities

| 缁勪欢 | 璇█ | 绔彛 | 鑱岃矗 |
|------|------|------|------|
| DeepVector Server | C++17 | 8080 | 鍚戦噺瀛樺偍銆佺储寮曘€佹悳绱€佹寔涔呭寲 |
| Agent Server | Python 3.11+ | 8090 | LLM 浜や簰銆佹煡璇㈣鍒掋€佸杞绱€丮CP |
| Ollama (鍙€? | Go | 11434 | 鏈湴 LLM 鎺ㄧ悊 |
| OpenAI API (鍙€? | 浜戠 | - | 浜戠 LLM + 宓屽叆 |

---

## 2. 鐜瑕佹眰 / Prerequisites

### 纭欢瑕佹眰 / Hardware Requirements

| 閰嶇疆 | 鏈€浣?| 鎺ㄨ崘 |
|------|------|------|
| CPU | 4 鏍?| 8 鏍? (鏀寔 AVX2) |
| RAM | 8 GB | 16 GB (鏈湴 LLM 闇€瑕? |
| 纾佺洏 | 10 GB | 50 GB+ (SSD 鎺ㄨ崘) |
| GPU (鍙€? | - | NVIDIA + CUDA (鍔犻€熸湰鍦?LLM) |

### 杞欢瑕佹眰 / Software Requirements

| 杞欢 | 鐗堟湰 | 鐢ㄩ€?|
|------|------|------|
| Python | 3.11+ | Agent 灞?|
| CMake | 3.16+ | C++ 鏋勫缓 |
| GCC | g++-12 (Linux/WSL2) | C++ 缂栬瘧 |
| Ninja | 1.10+ (鍙€? | 鍔犻€?C++ 鏋勫缓 |
| Ollama | 0.32+ (鍙€? | 鏈湴 LLM |
| Docker | 24+ (鍙€? | 瀹瑰櫒鍖栭儴缃?|

### Windows 鐗规畩璇存槑

Windows 寮€鍙戦渶瑕?WSL2 (Ubuntu 22.04):

```powershell
# 1. 瀹夎 WSL2
wsl --install -d Ubuntu-22.04

# 2. 鍦?WSL2 涓畨瑁呬緷璧?wsl
sudo apt update
sudo apt install -y g++-12 cmake ninja-build
```

---

## 3. 瀹夎鎸囧崡 / Installation

### Step 1: 鍏嬮殕浠撳簱 / Clone Repository

```bash
git clone --recursive https://github.com/Thezx-a/DeepVector.git
cd DeepVector
```

濡傛灉宸茬粡鍏嬮殕浣嗗瓙妯″潡鏈垵濮嬪寲:

```bash
git submodule update --init --recursive
```

### Step 2: 瀹夎 Python 渚濊禆 / Install Python Dependencies

```bash
# 鏍稿績渚濊禆 / Core dependencies
pip install httpx pydantic sentence-transformers

# 鈿狅笍 瀹夎鎱㈢殑瑙ｅ喅鏂规 / If slow, use mirror:
pip install -i https://pypi.tuna.tsinghua.edu.cn/simple httpx pydantic sentence-transformers

# 鍙€? FastAPI 鏈嶅姟鍣?(鎺ㄨ崘) / For FastAPI server (recommended)
pip install fastapi uvicorn

# 鍙€? MCP Server / For MCP protocol support
pip install mcp

# 鍙€? 娴嬭瘯妗嗘灦 / For running tests
pip install pytest pytest-asyncio
```

### Step 3: 缂栬瘧 C++ Server / Build C++ Server

**Linux / WSL2:**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-12
cmake --build build --target lumendb_server -j$(nproc)
```

**Windows (鍘熺敓 MSVC) 鏆備笉鏀寔**, 璇峰湪 WSL2 涓瀯寤恒€?
楠岃瘉缂栬瘧鎴愬姛:

```bash
./build/server/lumendb_server --help
# 棰勬湡杈撳嚭: 鏄剧ず鍛戒护琛屽弬鏁伴€夐」
```

### Step 4: 瀹夎 Ollama (鍙€? / Install Ollama (Optional)

```bash
# Linux / WSL2
curl -fsSL https://ollama.com/install.sh | sh

# 鎷夊彇宓屽叆妯″瀷 (蹇呴渶) / Pull embedding model (required)
ollama pull nomic-embed-text

# 鎷夊彇 LLM 妯″瀷 / Pull LLM model
ollama pull qwen2.5:3b
# 鎴栨洿澶х殑妯″瀷 / Or larger model:
# ollama pull qwen2.5:7b

# 楠岃瘉 / Verify
ollama list
# 搴旀樉绀? nomic-embed-text 鍜?qwen2.5:3b
```

> 鈿狅笍 **缃戠粶鎱㈡€庝箞鍔?** 浣跨敤浠ｇ悊鎴栧浗鍐呴暅鍍?
> ```bash
> # 璁剧疆浠ｇ悊 / Set proxy
> export http_proxy=http://127.0.0.1:7890
> export https_proxy=http://127.0.0.1:7890
> ```

---

## 4. 閰嶇疆璇存槑 / Configuration

### 鐜鍙橀噺 / Environment Variables

鎵€鏈夐厤缃彲閫氳繃鐜鍙橀噺瑕嗙洊:

| 鍙橀噺 | 榛樿鍊?| 璇存槑 |
|------|--------|------|
| `AGENTICDB_LLM_PROVIDER` | `ollama` | `openai` 鎴?`ollama` |
| `AGENTICDB_LLM_MODEL` | `qwen2.5:7b` | LLM 妯″瀷鍚嶇О |
| `AGENTICDB_EMBEDDING_PROVIDER` | `local` | `local` 鎴?`openai` |
| `AGENTICDB_EMBEDDING_MODEL` | `all-MiniLM-L6-v2` | 鏈湴宓屽叆妯″瀷 |
| `OPENAI_API_KEY` | - | OpenAI API 瀵嗛挜 |
| `AGENTICDB_DEEPVECTOR_URL` | `http://localhost:8080` | DeepVector 鍦板潃 |
| `AGENTICDB_AGENT_PORT` | `8090` | Agent 鏈嶅姟绔彛 |
| `AGENTICDB_MAX_ROUNDS` | `5` | 鏈€澶ф绱㈣疆鏁?|
| `AGENTICDB_QUALITY_THRESHOLD` | `0.7` | 璐ㄩ噺璇勫垎闃堝€?|

### 閰嶇疆鏂囦欢 / Config File

涔熷彲浠ラ€氳繃 Python 浠ｇ爜閰嶇疆:

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

## 5. 鍚姩杩愯 / Running

### 鏈€灏忓惎鍔?(DeepVector + 鏈湴宓屽叆, 鏃?LLM)

```bash
# 缁堢 1: 鍚姩 DeepVector
cd DeepVector
./build/server/lumendb_server --port 8080 --dim 384 --data-dir ./data

# 楠岃瘉
curl http://localhost:8080/health
# {"status":"ok","vectors":0,"dim":384}
```

### 瀹屾暣鍚姩 (DeepVector + Agent + Ollama)

```bash
# 缁堢 1: 鍚姩 Ollama (濡傛灉浣跨敤鏈湴 LLM)
ollama serve

# 缁堢 2: 鍚姩 DeepVector
./build/server/lumendb_server --port 8080 --dim 384 --data-dir ./data

# 缁堢 3: 鍚姩 Agent Server
cd DeepVector
python agent/server/app.py
# 鍚姩 FastAPI 鏈嶅姟鍣? http://0.0.0.0:8090

# 缁堢 4: 鐏屽叆鏁版嵁 + 杩愯 Demo
python scripts/demo_data.py
python examples/demo_agentic_search.py
```

### 浣跨敤 OpenAI (鏃犻渶 Ollama)

```bash
# 缁堢 1: 鍚姩 DeepVector
./build/server/lumendb_server --port 8080 --dim 1536 --data-dir ./data

# 缁堢 2: 鍚姩 Agent Server (OpenAI 妯″紡)
export AGENTICDB_LLM_PROVIDER=openai
export AGENTICDB_LLM_MODEL=gpt-4o
export AGENTICDB_EMBEDDING_PROVIDER=openai
export AGENTICDB_EMBEDDING_MODEL=text-embedding-3-small
export OPENAI_API_KEY=sk-your-key-here

python agent/server/app.py
```

### Docker 閮ㄧ讲 / Docker Deployment

```bash
# 鏋勫缓闀滃儚 / Build image
docker build -t lumendb:latest .

# 鍚姩瀹瑰櫒 / Run container
docker run -d \
  --name lumendb \
  -p 8080:8080 \
  -v ./data:/data \
  -e DEEPVECTOR_DIM=384 \
  lumendb:latest
```

---

## 6. API 鍙傝€?/ API Reference

### DeepVector C++ Server (port 8080)

#### `GET /health`
鍋ュ悍妫€鏌?/ Health check.

```bash
curl http://localhost:8080/health
# {"status":"ok","vectors":1024,"dim":384}
```

#### `POST /search`
鍚戦噺鎼滅储 / Vector search.

```bash
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"vector": [0.1, 0.2, ...], "k": 10}'
# {"results":[{"id":1,"distance":0.23},...]}
```

鏀寔杩囨护 / With filter:

```bash
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"vector": [...], "k": 10, "filter": {"op":"eq","field":"tags","value":"RAG"}}'
```

#### `POST /insert`
鎻掑叆鍚戦噺 / Insert vector.

```bash
curl -X POST http://localhost:8080/insert \
  -H "Content-Type: application/json" \
  -d '{"vector": [0.1, 0.2, ...]}'
# {"ids":[1]}
```

鎵归噺鎻掑叆 / Batch insert:

```bash
curl -X POST http://localhost:8080/insert \
  -H "Content-Type: application/json" \
  -d '{"vectors": [[0.1, ...], [0.2, ...]]}'
```

#### `GET /collections`
鍒楀嚭闆嗗悎鍒楄〃 / List collections.

```bash
curl http://localhost:8080/collections
# {"collections":[{"name":"default","vectors":1024,"dim":384}]}
```

#### `POST /batch/search`
鎵归噺鎼滅储 / Batch search.

```bash
curl -X POST http://localhost:8080/batch/search \
  -H "Content-Type: application/json" \
  -d '{"queries": [{"vector": [...], "k": 5}, {"vector": [...], "k": 5}]}'
```

#### `DELETE /vectors/:id`
鍒犻櫎鍚戦噺 / Delete vector.

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
瀹屾暣 Agent 妫€绱?/ Full agent search.

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
绠€娲侀棶绛?/ Simple Q&A.

```bash
curl -X POST http://localhost:8090/ask \
  -H "Content-Type: application/json" \
  -d '{"question": "Explain HNSW"}'
# {"answer": "HNSW is Hierarchical Navigable Small World...", "rounds": 1}
```

#### `POST /plan`
浠呮煡鐪嬫绱㈣鍒?(涓嶆墽琛? / Preview plan only.

```bash
curl -X POST http://localhost:8090/plan \
  -H "Content-Type: application/json" \
  -d '{"question": "Compare HNSW and IVF"}'
# {"strategy": "multi_query", "reasoning": "Multi-part comparison", "steps": 2}
```

---

## 7. 鏁版嵁绠＄悊 / Data Management

### 鐏屽叆绀轰緥鏁版嵁 / Insert Demo Data

```bash
python scripts/demo_data.py
# 棰勬湡杈撳嚭:
#   Inserting 15 documents into DeepVector...
#   [1/15] Inserted doc 1
#   [2/15] Inserted doc 2
#   ...
#   Done!
```

### 鑷畾涔夋暟鎹泦 / Custom Dataset

```python
from agent.embedding.service import EmbeddingService
import httpx
import numpy as np

# 鍑嗗鏁版嵁 / Prepare data
documents = [
    "Document 1 text...",
    "Document 2 text...",
]

# 宓屽叆 / Embed
svc = EmbeddingService()
vectors = await svc.embed(documents)

# 鎻掑叆 / Insert
async with httpx.AsyncClient() as client:
    for vec in vectors:
        resp = await client.post(
            "http://localhost:8080/insert",
            json={"vector": vec},
        )
        print(f"Inserted: {resp.json()['ids'][0]}")
```

### 鏁版嵁鎸佷箙鍖?/ Data Persistence

DeepVector 浣跨敤 mmap 鎸佷箙鍖栧埌纾佺洏銆傛暟鎹洰褰曠粨鏋?

```
./data/
鈹溾攢鈹€ vectors.bin        # 鍚戦噺鏁版嵁 (mmap)
鈹溾攢鈹€ docs/              # 鍏冩暟鎹?(MiniKV LSM-Tree)
鈹?  鈹溾攢鈹€ MANIFEST-00001
鈹?  鈹溾攢鈹€ CURRENT
鈹?  鈹斺攢鈹€ *.sst
鈹斺攢鈹€ *.cfg.json         # 闆嗗悎閰嶇疆
```

澶囦唤: 鐩存帴澶嶅埗 data/ 鐩綍鍗冲彲銆?
---

## 8. 鏁呴殰鎺掗櫎 / Troubleshooting

### 甯歌闂 / Common Issues

#### Q: DeepVector 鍚姩澶辫触 "failed to bind to port"

```bash
# 妫€鏌ョ鍙ｅ崰鐢?/ Check port usage
netstat -ano | grep 8080

# 浣跨敤涓嶅悓绔彛 / Use different port
./build/server/lumendb_server --port 8081
```

#### Q: Python 瀵煎叆 agent 鍖呭け璐?/ ImportError

```bash
# 纭繚鍦?DeepVector 鐩綍涓嬭繍琛?/ Run from DeepVector directory
cd DeepVector

# 妫€鏌?PYTHONPATH / Check Python path
$env:PYTHONPATH = "$env:PYTHONPATH;."
python -c "from agent.config import load_config; print(load_config())"
```

#### Q: Ollama 杩炴帴琚嫆缁?/ Connection refused

```bash
# 妫€鏌?Ollama 鏄惁杩愯 / Check if Ollama is running
ollama list

# 鍚姩 Ollama 鏈嶅姟 / Start Ollama
ollama serve &

# 璁剧疆 Ollama 鍦板潃 / Set Ollama host
set AGENTICDB_OLLAMA_HOST=http://127.0.0.1:11434
```

#### Q: 妫€绱㈢粨鏋滀负绌虹殑甯歌鍘熷洜 / Empty Search Results

1. 宓屽叆妯″瀷缁村害涓嶅尮閰? DeepVector `--dim` 蹇呴』涓?embedding 杈撳嚭缁村害涓€鑷?   - all-MiniLM-L6-v2: 384
   - text-embedding-3-small: 1536
   - nomic-embed-text: 768
2. 娌℃湁鏁版嵁: 鍏堣繍琛?`python scripts/demo_data.py`
3. DeepVector 鏈惎鍔? 妫€鏌?`curl http://localhost:8080/health`

#### Q: C++ 缂栬瘧閿欒 / Build Errors

```bash
# 娓呯悊閲嶈瘯 / Clean rebuild
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target lumendb_server

# 妫€鏌ョ紪璇戝櫒 / Check compiler
g++-12 --version
```

---

## 9. 鎬ц兘浼樺寲 / Performance Tuning

### C++ Server 浼樺寲

| 鍙傛暟 | 榛樿 | 璇存槑 |
|------|------|------|
| `--dim` | 768 | 鍚戦噺缁村害, 瓒婂皬鎼滅储瓒婂揩 |
| HNSW `M` | 16 | 鍥炬渶澶ч偦灞呮暟, 瓒婂ぇ鍙洖鐜囪秺楂樹絾鍐呭瓨鍜屽欢杩熷鍔?|
| HNSW `ef_search` | 50 | 鎼滅储瀹藉害, 瓒婂ぇ鍙洖鐜囪秺楂樹絾寤惰繜澧炲姞 |
| `USE_AVX2` | ON | 鍚敤 SIMD 鍔犻€?(缂栬瘧閫夐」) |

### Agent Server 浼樺寲

| 鍙傛暟 | 榛樿 | 浼樺寲寤鸿 |
|------|------|---------|
| `max_rounds` | 5 | 绠€鍗曟煡璇㈣涓?1-2, 澶嶆潅鏌ヨ 3-5 |
| `quality_threshold` | 0.7 | 璐ㄩ噺瑕佹眰楂樿涓?0.8, 閫熷害蹇涓?0.5 |
| `temperature` | 0.1 | 闇€瑕佺‘瀹氭€ц涓?0.0, 闇€瑕佸垱鎰忚涓?0.7 |
| `top_k_final` | 10 | 鏈€缁堣繑鍥炵粨鏋滄暟, 涓嶅奖鍝嶈川閲忎絾褰卞搷 token 娑堣€?|

### 鎵瑰鐞?/ Batch Processing

```bash
# 鎵归噺鎻掑叆 10000 涓悜閲?python -c "
import httpx, numpy as np
vectors = np.random.randn(10000, 384).astype(np.float32)
for batch in np.array_split(vectors, 10):
    resp = httpx.post('http://localhost:8080/insert',
        json={'vectors': batch.tolist()})
    print(f'Inserted batch: {resp.json()}')
"
```

---

## 10. 鐢熶骇閮ㄧ讲妫€鏌ユ竻鍗?/ Production Checklist

### 瀹夊叏 / Security
- [ ] 璁剧疆 DeepVector `--api-key` 闃叉鏈巿鏉冭闂?- [ ] 浣跨敤鐜鍙橀噺绠＄悊瀵嗛挜, 涓嶇‖缂栫爜
- [ ] 灏?Agent Server 缃簬鍙嶅悜浠ｇ悊鍚?(nginx/Caddy)
- [ ] 鍚敤 HTTPS (Let's Encrypt)

### 鍙潬鎬?/ Reliability
- [ ] 浣跨敤 systemd/supervisor 绠＄悊杩涚▼, 鏀寔鑷姩閲嶅惎
- [ ] 閰嶇疆鏁版嵁瀹氭湡澶囦唤
- [ ] 璁剧疆璧勬簮闄愬埗 (ulimit/RLIMIT)
- [ ] 鐩戞帶: 鎺ュ叆 Prometheus + Grafana

### 鎬ц兘 / Performance
- [ ] 浣跨敤 GPU 鍔犻€熸湰鍦?LLM (Ollama + CUDA)
- [ ] 浣跨敤 SSD 瀛樺偍鍚戦噺鏁版嵁
- [ ] 璋冩暣 HNSW 鍙傛暟骞宠　鍙洖鐜囧拰寤惰繜
- [ ] 鍚敤鍝嶅簲缂撳瓨 (Redis) 鍑忓皯閲嶅鏌ヨ

### 杩愮淮 / Operations
- [ ] 閰嶇疆鏃ュ織杞浆 (logrotate)
- [ ] 璁剧疆鍋ュ悍妫€鏌ョ鐐瑰拰鍛婅
- [ ] 鍑嗗瀹归噺瑙勫垝鎸囧崡
- [ ] 缂栧啓鏁呴殰鎭㈠ SOP

### 绀轰緥 systemd 鏈嶅姟 / Example systemd Service

```ini
# /etc/systemd/system/deepvector.service
[Unit]
Description=DeepVector Vector Database Server
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
