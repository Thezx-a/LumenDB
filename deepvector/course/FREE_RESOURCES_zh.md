# 免费资源指南 · 不花钱跑通 DeepVector

> 参考 [Hello-Agents 环境配置](https://github.com/datawhalechina/hello-agents/blob/main/Extra-Chapter/Extra07-%E7%8E%AF%E5%A2%83%E9%85%8D%E7%BD%AE.md) 的写法：  
> **先选一种方案 → 注册拿 Key（或用 Ollama）→ 复制 `.env` → 跑一遍下面的验证命令。**

Agent 这边和 Hello-Agents 一样，走 **OpenAI 兼容接口**：把 `AGENTICDB_LLM_PROVIDER` 设成 `openai`，再填 `OPENAI_BASE_URL` 和 Key 就行。  
文本转向量默认用本机 **sentence-transformers**，不用额外买 embedding API。

---

## 0. 三种常见组合（选一个就行）

| 方案 | 大模型从哪来 | 向量从哪来 | 适合什么情况 |
|------|-------------|-----------|-------------|
| **A 全本地（最省事）** | [Ollama](https://ollama.com/) | 本机 `all-MiniLM-L6-v2` | 内存 8GB+，不想注册任何网站 |
| **B 云端大模型 + 本机向量** | ModelScope / 硅基流动 / Groq | 本机 384 维 | 电脑跑不动 7B 模型 |
| **C Docker** | 同 A 或 B | 同 A | Windows 不想自己编译 C++ |

只学 **C++ 向量库（Track A）** 的话，不用 LLM，装好 CMake 和编译器就够了。

---

## 1. 嵌入（Embedding）——默认不花钱

| 方式 | 说明 | 向量维度 | 怎么开 |
|------|------|---------|--------|
| **本机（默认）** | [all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2) | **384** | `AGENTICDB_EMBEDDING_PROVIDER=local` |
| 国内下载慢 | 用 [hf-mirror.com](https://hf-mirror.com/) 镜像 | 同上 | `HF_ENDPOINT=https://hf-mirror.com` |

第一次跑会自动下载大约 90MB 模型。  
**重要：** C++ 服务启动时要写对维度，否则搜索会报错或结果不对：

```bash
./deepvector_server --port 8080 --dim 384
```

新手建议一直用 **本机嵌入 + 384 维**。别同时换成 OpenAI embedding（1536 维），不然还得改 `--dim` 并重新灌数据。

---

## 2. 免费大模型 API（都能接进本仓库）

下面这些都支持 `AGENTICDB_LLM_PROVIDER=openai` + `OPENAI_BASE_URL`。  
**免费额度各平台会变，以官网控制台为准。**

| 平台 | 注册地址 | Base URL | 免费情况（参考） | 本仓库常用模型 |
|------|---------|----------|-----------------|---------------|
| **Ollama** | [ollama.com](https://ollama.com/) | `http://localhost:11434/v1` | 本机完全免费 | `qwen2.5:7b`（**默认**） |
| **ModelScope 魔搭** | [modelscope.cn](https://modelscope.cn/) | `https://api-inference.modelscope.cn/v1/` | Hello-Agents 文档写约 2000 次/天 | `Qwen/Qwen2.5-7B-Instruct` |
| **硅基流动** | [siliconflow.cn](https://siliconflow.cn/) | `https://api.siliconflow.cn/v1` | 新用户有赠送额度 | `Qwen/Qwen2.5-7B-Instruct` |
| **Groq** | [console.groq.com](https://console.groq.com/) | `https://api.groq.com/openai/v1` | 有免费档，限 RPM/RPD | `llama-3.3-70b-versatile` |
| **OpenRouter** | [openrouter.ai](https://openrouter.ai/) | `https://openrouter.ai/api/v1` | `:free` 模型有日限额 | 站内搜 `*:free` |
| **Google AI Studio** | [aistudio.google.com](https://aistudio.google.com/) | 见 Google 文档 | 有免费配额 | `gemini-2.0-flash` 等 |
| **AIHubmix** | [aihubmix.com](https://aihubmix.com/) | `https://aihubmix.com/v1` | 模型列表里标「免费」的 | `coding-glm-4.7-free` |

学习阶段建议：**能装 Ollama 就用 Ollama**；装不了再用 **ModelScope**（和 Hello-Agents 教程同一套流程）。

### ModelScope 注册（和 Hello-Agents 一样）

1. 注册 [ModelScope](https://modelscope.cn/)  
2. 打开 **模型服务 → API 推理**，按提示 [绑定阿里云账号](https://modelscope.cn/docs/accounts/aliyun-binding-and-authorization)（不绑定 API 会不可用）  
3. 创建 **SDK 令牌**，就是 API Key，一般以 `ms-` 开头  
4. 在 [模型库](https://modelscope.cn/models?filter=inference_type) 里选带 **API-Inference** 的 Qwen 模型  

---

## 3. `.env` 怎么写

在 `deepvector/` 下新建 `.env`（**不要提交到 Git**）：

### A：Ollama（一分钱不花）

```env
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
AGENTICDB_LLM_PROVIDER=ollama
AGENTICDB_LLM_MODEL=qwen2.5:7b
AGENTICDB_OLLAMA_HOST=http://localhost:11434
AGENTICDB_EMBEDDING_PROVIDER=local
```

```bash
ollama pull qwen2.5:7b
ollama serve   # 托盘程序没起来时手动跑
```

### B：ModelScope + 本机嵌入

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=ms-你的令牌
OPENAI_BASE_URL=https://api-inference.modelscope.cn/v1/
AGENTICDB_LLM_MODEL=Qwen/Qwen2.5-7B-Instruct
AGENTICDB_EMBEDDING_PROVIDER=local
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
```

### C：硅基流动

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=sk-你的密钥
OPENAI_BASE_URL=https://api.siliconflow.cn/v1
AGENTICDB_LLM_MODEL=Qwen/Qwen2.5-7B-Instruct
AGENTICDB_EMBEDDING_PROVIDER=local
```

### D：Groq

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=gsk_你的密钥
OPENAI_BASE_URL=https://api.groq.com/openai/v1
AGENTICDB_LLM_MODEL=llama-3.3-70b-versatile
AGENTICDB_EMBEDDING_PROVIDER=local
```

Linux/macOS/WSL 加载环境变量后启动：

```bash
cd deepvector
export $(grep -v '^#' .env | xargs)
python -m agent.server.app
```

Windows PowerShell：

```powershell
$env:AGENTICDB_LLM_PROVIDER = "openai"
$env:OPENAI_API_KEY = "ms-xxxx"
$env:OPENAI_BASE_URL = "https://api-inference.modelscope.cn/v1/"
$env:AGENTICDB_LLM_MODEL = "Qwen/Qwen2.5-7B-Instruct"
python -m agent.server.app
```

更多组合见 [`deepvector/.env.example`](../.env.example)。

---

## 4. 其他免费工具

| 工具 | 链接 | 干什么用 |
|------|------|---------|
| Docker Desktop | [下载页](https://www.docker.com/products/docker-desktop/) | Windows 不编译也能跑 |
| WSL2 | [微软文档](https://learn.microsoft.com/windows/wsl/install) | Windows 上编译 C++ |
| HuggingFace | [huggingface.co](https://huggingface.co/) | 下嵌入模型 |
| HF 镜像 | [hf-mirror.com](https://hf-mirror.com/) | 国内加速下载 |

仓库根目录 `docker compose up` 会同时起 C++（8080）和 Agent（8090），细节见 [RUN.md](../../RUN.md)。

---

## 5. 想继续学 Agent 可以看

| 资源 | 链接 |
|------|------|
| Hello-Agents 教程 | [GitHub](https://github.com/datawhalechina/hello-agents) · [在线阅读](https://hello-agents.datawhale.cc/) |
| Datawhale | [datawhale.cn](https://www.datawhale.cn/) |
| MCP 协议 | [modelcontextprotocol.io](https://modelcontextprotocol.io/) |
| HNSW 论文 | [arXiv:1603.09320](https://arxiv.org/abs/1603.09320) |

---

## 6. 跑通了吗？按顺序执行

```bash
# 1. 启动 C++ 向量库（或 docker compose up）
./build/deepvector/deepvector_server --port 8080 --dim 384 &
curl -s http://127.0.0.1:8080/health

# 2. 灌示例数据（只用本机 embedding，不耗 LLM 额度）
cd deepvector && pip install -r requirements.txt
python scripts/demo_data.py

# 3. 启动 Agent
python -m agent.server.app &
curl -s http://127.0.0.1:8090/health

# 4. 问一个问题（Ollama 不扣云端额度；云端 API 会计数）
curl -s -X POST http://127.0.0.1:8090/ask \
  -H 'Content-Type: application/json' \
  -d '{"question":"什么是 RAG？"}'
```

只想测大模型 API 通不通（不用启动 DeepVector）：

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

## 7. 出错了怎么办

| 你看到的 | 多半是因为 | 怎么办 |
|---------|-----------|--------|
| `429 Too Many Requests` | 免费 API 超配额 | 换 Ollama，或等第二天 |
| ModelScope 提示 API 不可用 | 没绑阿里云 | 按官网绑定后再试 |
| 搜索报维度不对 | embedding 维度和 `--dim` 不一致 | 用 local + `--dim 384` |
| 连不上 Ollama | 服务没开 | `ollama serve`，再 `ollama pull qwen2.5:7b` |
| 模型下载很慢 | HuggingFace 网络 | `export HF_ENDPOINT=https://hf-mirror.com` |

---

## 8. 接下来看哪

- 完整启动步骤 → [RUN.md](../../RUN.md)  
- 环境变量从哪读 → [ch03_config](ch03_config/03_配置系统_zh.md)  
- LLM 代码在哪 → `deepvector/agent/llm/router.py`  
- 面试题 → [INTERVIEW_BANK.md](INTERVIEW_BANK.md)
