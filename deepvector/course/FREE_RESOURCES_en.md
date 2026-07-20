# Free Resources · Run DeepVector at Zero Cost

> Same spirit as [Hello-Agents Extra07 (Environment Setup)](https://github.com/datawhalechina/hello-agents/blob/main/Extra-Chapter/Extra07-%E7%8E%AF%E5%A2%83%E9%85%8D%E7%BD%AE.md):  
> **Pick a free stack → register for a key → copy `.env` → run the smoke test.**

The Agent layer speaks **OpenAI-compatible APIs** (`provider=openai` + `OPENAI_BASE_URL`), matching Hello-Agents. Embeddings default to **local sentence-transformers** (no API spend).

---

## 0. Three zero-cost stacks (pick one)

| Stack | LLM | Embedding | Best for | GPU |
|-------|-----|-----------|----------|-----|
| **A Local (recommended)** | [Ollama](https://ollama.com/) | local `all-MiniLM-L6-v2` | 8GB+ RAM laptop | optional |
| **B Cloud LLM + local embed** | ModelScope / SiliconFlow / Groq | local 384-d | weak GPU / CPU only | no |
| **C Docker** | A or B | A | Windows without native compile | no |

**Track A (C++ engine)** needs no LLM API—only CMake and a compiler.

---

## 1. Embeddings (free by default)

| Mode | Source | Dim | Config |
|------|--------|-----|--------|
| **Local (default)** | [HuggingFace all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2) | **384** | `AGENTICDB_EMBEDDING_PROVIDER=local` |
| HF mirror (CN) | [hf-mirror.com](https://hf-mirror.com/) | same | `HF_ENDPOINT=https://hf-mirror.com` |

First run downloads ~90MB. C++ server must match:

```bash
./deepvector_server --port 8080 --dim 384
```

> Beginners: keep **local embedding + dim 384**. Switching to OpenAI embeddings (1536-d) requires `--dim 1536` and re-ingesting data.

---

## 2. Free LLM APIs (OpenAI-compatible)

All work with `AGENTICDB_LLM_PROVIDER=openai` + `OPENAI_BASE_URL`.

| Provider | Sign up | Base URL | Free tier (approx.) | Suggested model | Notes |
|----------|---------|----------|---------------------|-----------------|-------|
| **Ollama** | [ollama.com](https://ollama.com/) | `http://localhost:11434/v1` | unlimited local | `qwen2.5:7b` | **Repo default** |
| **ModelScope** | [modelscope.cn](https://modelscope.cn/) | `https://api-inference.modelscope.cn/v1/` | ~2000 calls/day | `Qwen/Qwen2.5-7B-Instruct` | bind Aliyun account |
| **SiliconFlow** | [siliconflow.cn](https://siliconflow.cn/) | `https://api.siliconflow.cn/v1` | trial + free models | `Qwen/Qwen2.5-7B-Instruct` | common in CN Agent tutorials |
| **Groq** | [console.groq.com](https://console.groq.com/) | `https://api.groq.com/openai/v1` | rate-limited free tier | `llama-3.3-70b-versatile` | very fast inference |
| **OpenRouter** | [openrouter.ai](https://openrouter.ai/) | `https://openrouter.ai/api/v1` | ~50 req/day on `:free` models | see `*:free` on site | model id must end with `:free` |
| **Google AI Studio** | [aistudio.google.com](https://aistudio.google.com/) | Gemini OpenAI-compatible | quota varies | `gemini-2.0-flash` | policy changes |
| **AIHubmix** | [aihubmix.com](https://aihubmix.com/) | `https://aihubmix.com/v1` | free-tagged models | `coding-glm-4.7-free` | Hello-Agents Extra07 pick |

Quotas change—check each console. For learning, **Ollama** or **ModelScope** are the most documented.

---

## 3. Copy-paste `.env` examples

Create `deepvector/.env` (never commit secrets):

### A — Ollama (zero API cost)

```env
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
AGENTICDB_LLM_PROVIDER=ollama
AGENTICDB_LLM_MODEL=qwen2.5:7b
AGENTICDB_OLLAMA_HOST=http://localhost:11434
AGENTICDB_EMBEDDING_PROVIDER=local
```

### B — ModelScope + local embedding

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=ms-your-token
OPENAI_BASE_URL=https://api-inference.modelscope.cn/v1/
AGENTICDB_LLM_MODEL=Qwen/Qwen2.5-7B-Instruct
AGENTICDB_EMBEDDING_PROVIDER=local
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
```

### C — SiliconFlow

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=sk-your-key
OPENAI_BASE_URL=https://api.siliconflow.cn/v1
AGENTICDB_LLM_MODEL=Qwen/Qwen2.5-7B-Instruct
AGENTICDB_EMBEDDING_PROVIDER=local
```

### D — Groq

```env
AGENTICDB_LLM_PROVIDER=openai
OPENAI_API_KEY=gsk_your-key
OPENAI_BASE_URL=https://api.groq.com/openai/v1
AGENTICDB_LLM_MODEL=llama-3.3-70b-versatile
AGENTICDB_EMBEDDING_PROVIDER=local
```

Full template: [`deepvector/.env.example`](../.env.example).

---

## 4. Free infrastructure

| Tool | Link | Use |
|------|------|-----|
| Docker Desktop | [docker.com](https://www.docker.com/products/docker-desktop/) | run both services on Windows |
| WSL2 | [Microsoft docs](https://learn.microsoft.com/windows/wsl/install) | compile C++ on Windows |
| HuggingFace | [huggingface.co](https://huggingface.co/) | embedding weights |
| HF mirror | [hf-mirror.com](https://hf-mirror.com/) | faster downloads in CN |

See [RUN.md](../../RUN.md) for `docker compose up`.

---

## 5. Free learning communities

| Resource | Link | Relation |
|----------|------|----------|
| Hello-Agents | [GitHub](https://github.com/datawhalechina/hello-agents) · [Docs](https://hello-agents.datawhale.cc/) | Agent patterns, MCP, env setup |
| Datawhale | [datawhale.cn](https://www.datawhale.cn/) | open course community |
| MCP spec | [modelcontextprotocol.io](https://modelcontextprotocol.io/) | Track B ch08 |

---

## 6. 5-minute smoke test

```bash
./build/deepvector/deepvector_server --port 8080 --dim 384 &
cd deepvector && pip install -r requirements.txt
python scripts/demo_data.py
python -m agent.server.app &
curl -s -X POST http://127.0.0.1:8090/ask \
  -H 'Content-Type: application/json' \
  -d '{"question":"What is RAG?"}'
```

---

## 7. Troubleshooting

| Symptom | Fix |
|---------|-----|
| HTTP 429 | free quota exhausted → use Ollama |
| ModelScope unusable | bind Aliyun account |
| dim mismatch | keep local embed + `--dim 384` |
| slow HF download | `HF_ENDPOINT=https://hf-mirror.com` |

Next → [RUN.md](../../RUN.md) · [ch03_config](ch03_config/03_配置系统_en.md)
