# Python Environment

## Version Requirements

- Python 3.9+, recommended 3.11
- pip 22+

## Installing Dependencies

```bash
# Core
pip install httpx pydantic sentence-transformers

# Server (recommended)
pip install fastapi uvicorn

# MCP Protocol
pip install mcp

# Testing
pip install pytest pytest-asyncio
```

> 💡 Use a mirror if slow: `pip install -i https://pypi.tuna.tsinghua.edu.cn/simple <pkg>`

## Virtual Environment

```bash
python -m venv .venv
source .venv/bin/activate  # Linux/WSL2
.venv\Scripts\activate     # Windows
```

## Verify Installation

```python
import httpx
import pydantic
from sentence_transformers import SentenceTransformer
model = SentenceTransformer("all-MiniLM-L6-v2")
vec = model.encode("Hello")
print(f"Dimension: {len(vec)}")  # 384
```

## Related Chapters

- 📎 **Ch05** [Embedding Service](../ch05_embedding/05_嵌入服务_en.md)
