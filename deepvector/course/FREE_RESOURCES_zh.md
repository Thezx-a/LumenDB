# 免费资源指南 · 零成本跑通 DeepVector

> 写法对齐 [Hello-Agents Extra07 环境配置](https://github.com/datawhalechina/hello-agents/blob/main/Extra-Chapter/Extra07-%E7%8E%AF%E5%A2%83%E9%85%8D%E7%BD%AE.md)：  
> **先选免费组合 → 注册拿 Key → 复制 `.env` → 跑通冒烟测试。**

本仓库 Agent 层使用 **OpenAI 兼容协议**（`provider=openai` + `OPENAI_BASE_URL`），与 Hello-Agents 的配置方式一致；嵌入默认走 **本地 sentence-transformers**，不花 API 钱。

---

## 0. 三种零成本组合（选一个即可）

| 方案 | LLM | Embedding | 适合谁 | 需要 GPU |
|------|-----|-----------|--------|----------|
| **A 完全本地（推荐）** | [Ollama](https://ollama.com/) | 本地 `all-MiniLM-L6-v2` | 有 8GB+ 内存的笔记本 | 可选 |
| **B 云端 LLM + 本地嵌入** | ModelScope / 硅基流动 / Groq | 本地 384 维 | 电脑跑不动大模型 | 否 |
| **C Docker 一键** | 同 A 或 B | 同 A | Windows 不想配编译环境 | 否 |

**C++ 向量库轨（Track A）** 不需要任何 LLM API，只要 CMake + 编译器即可。

---

## 1. 嵌入 Embedding（默认免费）

| 方式 | 说明 | 维度 | 配置 |
|------|------|------|------|
| **本地（默认）** | [HuggingFace all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2) | **384** | `AGENTICDB_EMBEDDING_PROVIDER=local` |
| HuggingFace 镜像 | 国内下载慢时用 [hf-mirror.com](https://hf-mirror.com/) | 同上 | `HF_ENDPOINT=https://hf-mirror.com` |

首次运行会自动下载约 90MB 模型。C++ 服务必须匹配维度：

```bash
./deepvector_server --port 8080 --dim 384
```

> 初学者请保持 **本地嵌入 + 384 维**，不要同时换 OpenAI embedding（1536 维），否则还要改 `--dim` 并重灌数据。

---

## 2. 免费 LLM API 清单（OpenAI 兼容）

以下均可通过 `AGENTICDB_LLM_PROVIDER=openai` + `OPENAI_BASE_URL` 接入本仓库 Agent。

| 平台 | 官网 / 注册 | Base URL | 免费额度（约） | 推荐模型 | 备注 |
|------|------------|----------|----------------|----------|------|
| **Ollama** | [ollama.com](https://ollama.com/) | `http://localhost:11434/v1` | 完全免费 | `qwen2.5:7b` | 本仓库**默认**；无需 Key |
| **ModelScope 魔搭** | [modelscope.cn](https://modelscope.cn/) | `https://api-inference.modelscope.cn/v1/` | 约 2000 次/天 | `Qwen/Qwen2.5-7B-Instruct` | 需绑定阿里云账号 |
| **硅基流动 SiliconFlow** | [siliconflow.cn](https://siliconflow.cn/) | `https://api.siliconflow.cn/v1` | 新用户赠额度 + 部分免费模型 | `Qwen/Qwen2.5-7B-Instruct` | Hello-Agents 生态常用 |
| **Groq** | [console.groq.com](https://console.groq.com/) | `https://api.groq.com/openai/v1` | 免费档 RPM/RPD 限制 | `llama-3.3-70b-versatile` | 推理极快，适合试 Agent |
| **OpenRouter** | [openrouter.ai](https://openrouter.ai/) | `https://openrouter.ai/api/v1` | 免费 `:free` 模型约 50 次/天 | 见站内 `*:free` 列表 | 模型 ID 需带 `:free` 后缀 |
| **Google AI Studio** | [aistudio.google.com](https://aistudio.google.com/) | Gemini OpenAI 兼容端点 | 免费档有配额 | `gemini-2.0-flash` 等 | 配额随 Google 政策变化 |
| **AIHubmix** | [aihubmix.com](https://aihubmix.com/) | `https://aihubmix.com/v1` | 有免费模型标签 | `coding-glm-4.7-free` | Hello-Agents Extra07 推荐 |

> 额度与政策会变动，以各平台控制台为准。学习阶段优先 **Ollama** 或 **ModelScope**，稳定且文档多。

### 注册步骤（以 ModelScope 为例，与 Hello-Agents 相同）

1. 注册 [ModelScope](https://modelscope.cn/) 账号  
2. 进入 **模型服务 → API 推理**，绑定 [阿里云账号](https://modelscope.cn/docs/accounts/aliyun-binding-and-authorization)  
3. 创建 **SDK 令牌**（即 API Key，形如 `ms-...`）  
4. 在 [模型库](https://modelscope.cn/models?filter=inference_type) 筛选 **API-Inference**，选带推理服务的 Qwen 模型  

---

## 3. 复制即用的 `.env` 示例

在 `deepvector/` 目录创建 `.env`（勿提交到 Git）：

### 方案 A：Ollama 本地（零 API 费用）

```env
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
AGENTICDB_LLM_PROVIDER=ollama
AGENTICDB_LLM_MODEL=qwen2.5:7b
AGENTICDB_OLLAMA_HOST=http://localhost:11434
AGENTICDB_EMBEDDING_PROVIDER=local
```

```bash
ollama pull qwen2.5:7b
ollama serve   # 若未自动启动
```

### 方案 B：ModelScope 云端 LLM + 本地嵌入

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=ms-你的令牌
OPENAI_BASE_URL=https://api-inference.modelscope.cn/v1/
AGENTICDB_LLM_MODEL=Qwen/Qwen2.5-7B-Instruct
AGENTICDB_EMBEDDING_PROVIDER=local
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
```

### 方案 C：硅基流动

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=sk-你的硅基流动密钥
OPENAI_BASE_URL=https://api.siliconflow.cn/v1
AGENTICDB_LLM_MODEL=Qwen/Qwen2.5-7B-Instruct
AGENTICDB_EMBEDDING_PROVIDER=local
```

### 方案 D：Groq

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=gsk_你的Groq密钥
OPENAI_BASE_URL=https://api.groq.com/openai/v1
AGENTICDB_LLM_MODEL=llama-3.3-70b-versatile
AGENTICDB_EMBEDDING_PROVIDER=local
```

加载 `.env`（可选，需 `pip install python-dotenv`，或手动 `export` / PowerShell `$env:`）：

```bash
cd deepvector
export $(grep -v '^#' .env | xargs)   # Linux/macOS/WSL
python -m agent.server.app
```

Windows PowerShell 示例：

```powershell
$env:AGENTICDB_LLM_PROVIDER = "openai"
$env:OPENAI_API_KEY = "ms-xxxx"
$env:OPENAI_BASE_URL = "https://api-inference.modelscope.cn/v1/"
$env:AGENTICDB_LLM_MODEL = "Qwen/Qwen2.5-7B-Instruct"
python -m agent.server.app
```

完整模板见 [`deepvector/.env.example`](../.env.example)。

---

## 4. 免费基础设施与工具

| 资源 | 链接 | 用途 |
|------|------|------|
| **Docker Desktop** | [docker.com/products/docker-desktop](https://www.docker.com/products/docker-desktop/) | Windows 免编译跑双服务 |
| **WSL2** | [Microsoft WSL 文档](https://learn.microsoft.com/windows/wsl/install) | Windows 上编译 C++ 服务 |
| **HuggingFace** | [huggingface.co](https://huggingface.co/) | 下载嵌入模型 |
| **HF 镜像** | [hf-mirror.com](https://hf-mirror.com/) | 国内加速模型下载 |
| **pytest** | `pip install pytest` | Agent 单元测试（免费） |

根目录 `docker compose up` 可一次拉起 C++ :8080 + Agent :8090，见 [RUN.md](../../RUN.md)。

---

## 5. 免费学习社区（扩展阅读）

| 资源 | 链接 | 与本教程关系 |
|------|------|--------------|
| **Hello-Agents** | [GitHub](https://github.com/datawhalechina/hello-agents) · [在线文档](https://hello-agents.datawhale.cc/) | Agent 范式、MCP、环境配置范例 |
| **Datawhale** | [datawhale.cn](https://www.datawhale.cn/) | 开源免费课程社区 |
| **Agent Learning Hub** | [GitHub](https://github.com/datawhalechina/Agent-Learning-Hub) | 2026 Agent 学习路线 |
| **MCP 协议** | [modelcontextprotocol.io](https://modelcontextprotocol.io/) | Track B `ch08_mcp` |
| **HNSW 论文** | [Malkov & Yashunin, 2018](https://arxiv.org/abs/1603.09320) | Track A `ch03` |

---

## 6. 5 分钟验证（确认免费栈可用）

```bash
# 1. C++ 服务（Track A 或 Docker）
./build/deepvector/deepvector_server --port 8080 --dim 384 &
curl -s http://127.0.0.1:8080/health

# 2. 灌示例数据（本地 embedding，不耗 LLM 额度）
cd deepvector && pip install -r requirements.txt
python scripts/demo_data.py

# 3. 启动 Agent
python -m agent.server.app &
curl -s http://127.0.0.1:8090/health

# 4. 问一句（会消耗 LLM 额度；Ollama 则不消耗）
curl -s -X POST http://127.0.0.1:8090/ask \
  -H 'Content-Type: application/json' \
  -d '{"question":"什么是 RAG？"}'
```

仅测 LLM 连通性（不启动 DeepVector）：

```python
from openai import OpenAI
client = OpenAI(
    api_key="你的Key",
    base_url="https://api-inference.modelscope.cn/v1/",
)
r = client.chat.completions.create(
    model="Qwen/Qwen2.5-7B-Instruct",
    messages=[{"role": "user", "content": "你好，回复 OK"}],
    max_tokens=10,
)
print(r.choices[0].message.content)
```

---

## 7. 常见问题

| 现象 | 原因 | 处理 |
|------|------|------|
| `429 Too Many Requests` | 免费 API 超配额 | 换 Ollama 或等次日重置 |
| ModelScope `api 无法使用` | 未绑定阿里云 | 按官方文档绑定账号 |
| 搜索维度错误 | embedding 维与 `--dim` 不一致 | 保持 local + `--dim 384` |
| Ollama 连接失败 | 服务未启动 | `ollama serve` + `ollama pull qwen2.5:7b` |
| 模型下载极慢 | HuggingFace 网络 | 设置 `HF_ENDPOINT=https://hf-mirror.com` |

---

## 8. 下一步

- 运行细节 → [RUN.md](../../RUN.md)  
- Agent 配置原理 → [ch03_config](ch03_config/03_配置系统_zh.md)  
- LLM Router 源码 → `deepvector/agent/llm/router.py`  
- 面试：免费栈 vs 生产栈权衡 → [INTERVIEW_BANK.md](INTERVIEW_BANK.md)
