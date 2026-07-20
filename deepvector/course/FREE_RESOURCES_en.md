# Free Resources · Run DeepVector Without Paying

> Same flow as [Hello-Agents Extra07](https://github.com/datawhalechina/hello-agents/blob/main/Extra-Chapter/Extra07-%E7%8E%AF%E5%A2%83%E9%85%8D%E7%BD%AE.md):  
> **Pick a stack → get a key (or use Ollama) → copy `.env` → run the smoke test.**

The Agent uses **OpenAI-compatible APIs** (`provider=openai` + `OPENAI_BASE_URL`). Embeddings default to **local sentence-transformers** (no API cost).

---

## 0. Three common stacks

| Stack | LLM | Embedding | When to use |
|-------|-----|-----------|-------------|
| **A Local (easiest)** | [Ollama](https://ollama.com/) | local `all-MiniLM-L6-v2` | 8GB+ RAM, no sign-ups |
| **B Cloud LLM + local embed** | ModelScope / SiliconFlow / Groq | local 384-d | PC can't run 7B models |
| **C Docker** | A or B | A | Windows, skip native compile |

**Track A (C++ only)** needs no LLM—just CMake and a compiler.

---

## 1. Embeddings (free by default)

| Mode | Source | Dim | Config |
|------|--------|-----|--------|
| **Local** | [all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2) | **384** | `AGENTICDB_EMBEDDING_PROVIDER=local` |
| CN mirror | [hf-mirror.com](https://hf-mirror.com/) | same | `HF_ENDPOINT=https://hf-mirror.com` |

First run downloads ~90MB. Start C++ with matching dim:

```bash
./deepvector_server --port 8080 --dim 384
```

Stick with **local + 384** until you know why you'd switch to OpenAI embeddings (1536-d).

---

## 2. Free LLM APIs

All work with `AGENTICDB_LLM_PROVIDER=openai` + `OPENAI_BASE_URL`. **Quotas change—check each provider's console.**

| Provider | Sign up | Base URL | Notes | Model |
|----------|---------|----------|-------|-------|
| **Ollama** | [ollama.com](https://ollama.com/) | `http://localhost:11434/v1` | fully local | `qwen2.5:7b` (default) |
| **ModelScope** | [modelscope.cn](https://modelscope.cn/) | `https://api-inference.modelscope.cn/v1/` | Hello-Agents docs cite ~2000 calls/day | `Qwen/Qwen2.5-7B-Instruct` |
| **SiliconFlow** | [siliconflow.cn](https://siliconflow.cn/) | `https://api.siliconflow.cn/v1` | trial credits | `Qwen/Qwen2.5-7B-Instruct` |
| **Groq** | [console.groq.com](https://console.groq.com/) | `https://api.groq.com/openai/v1` | rate-limited free tier | `llama-3.3-70b-versatile` |
| **OpenRouter** | [openrouter.ai](https://openrouter.ai/) | `https://openrouter.ai/api/v1` | `:free` models, daily cap | see `*:free` on site |
| **AIHubmix** | [aihubmix.com](https://aihubmix.com/) | `https://aihubmix.com/v1` | free-tagged models | `coding-glm-4.7-free` |

---

## 3. `.env` examples

See [`deepvector/.env.example`](../.env.example). Quick Ollama block:

```env
AGENTICDB_LLM_PROVIDER=ollama
AGENTICDB_LLM_MODEL=qwen2.5:7b
AGENTICDB_EMBEDDING_PROVIDER=local
AGENTICDB_DEEPVECTOR_URL=http://127.0.0.1:8080
```

---

## 4. Smoke test

```bash
./build/deepvector/deepvector_server --port 8080 --dim 384 &
cd deepvector && pip install -r requirements.txt && python scripts/demo_data.py
python -m agent.server.app &
curl -s -X POST http://127.0.0.1:8090/ask -H 'Content-Type: application/json' \
  -d '{"question":"What is RAG?"}'
```

---

## 5. Troubleshooting

| Symptom | Fix |
|---------|-----|
| HTTP 429 | quota hit → Ollama or wait |
| ModelScope unusable | bind Aliyun account |
| dim mismatch | local embed + `--dim 384` |
| slow HF download | `HF_ENDPOINT=https://hf-mirror.com` |

Next → [RUN.md](../../RUN.md) · [FREE_RESOURCES_zh.md](FREE_RESOURCES_zh.md) (full CN guide)
