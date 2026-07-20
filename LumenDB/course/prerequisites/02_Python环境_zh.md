# Python 环境 / Python Environment

## 版本要求 / Version Requirements

- Python 3.9+, 推荐 3.11
- pip 22+

## 依赖安装 / Installing Dependencies

```bash
# 核心 / Core
pip install httpx pydantic sentence-transformers

# 服务器 / Server (推荐)
pip install fastapi uvicorn

# MCP 协议 / MCP Protocol
pip install mcp

# 测试 / Testing
pip install pytest pytest-asyncio
```

> 💡 如果网络慢，使用国内镜像: `pip install -i https://pypi.tuna.tsinghua.edu.cn/simple <pkg>`

## 虚拟环境 / Virtual Environment

```bash
python -m venv .venv
source .venv/bin/activate  # Linux/WSL2
.venv\Scripts\activate     # Windows
```

## 验证安装 / Verify

```python
import httpx
import pydantic
from sentence_transformers import SentenceTransformer
model = SentenceTransformer("all-MiniLM-L6-v2")
vec = model.encode("Hello")
print(f"Dimension: {len(vec)}")  # 384
```

## 相关章节

- 📎 **Ch05** [嵌入服务](../ch05_embedding/05_嵌入服务_zh.md)
