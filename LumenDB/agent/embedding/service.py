"""
Embedding Service — 文本向量化的统一接口 / Unified text embedding interface.

设计原则 / Design Principles:
  1. 多后端支持: 本地 sentence-transformers 和 OpenAI API
  2. 延迟加载: 本地模型在首次使用时才加载 / Lazy model loading
  3. 缓存: 可选 MD5 缓存, 避免重复计算 / Optional MD5-based caching
  4. 批量处理: 自动分批以减少 API 调用 / Auto-batching for API efficiency

供应商差异 / Provider Differences:
  - 本地: 免费, 无网络依赖, 384维 / Free, no network dependency, 384-d
  - OpenAI: 按量付费, 更高精度, 1536维 / Pay-per-use, higher quality, 1536-d
  
性能说明 / Performance Notes:
  - 本地 all-MiniLM-L6-v2: ~100 text/s (CPU), ~500 text/s (GPU)
  - OpenAI text-embedding-3-small: ~500 text/min (API rate limits)
"""

import hashlib
import json
import logging
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

import httpx

from ..config import EmbeddingConfig

logger = logging.getLogger(__name__)


class EmbeddingService:
    """
    文本嵌入服务 / Text Embedding Service.

    支持本地 sentence-transformers 和 OpenAI API 两种模式。
    提供 embed() 批量接口和 embed_single() 单条接口。

    用法 / Usage:
        config = EmbeddingConfig(provider="local")
        svc = EmbeddingService(config)

        # 批量嵌入 / Batch embedding
        vectors = await svc.embed(["Hello world", "Another text"])
        print(len(vectors[0]))  # 384 for all-MiniLM-L6-v2

        # 单条嵌入 / Single text
        vec = await svc.embed_single("What is RAG?")
    """

    def __init__(self, config: EmbeddingConfig):
        """
        初始化嵌入服务 / Initialize embedding service.

        Args:
            config: 嵌入配置, 包含 provider/model/keys 等
        """
        self.config = config
        self._local_model: Any = None  # 延迟加载 / Lazy load
        self._client: Optional[httpx.AsyncClient] = None
        self._cache: Dict[str, List[float]] = {}
        self._cache_path = Path(".embedding_cache.json")

    def _load_local_model(self):
        """
        加载本地 sentence-transformers 模型 / Load local model.

        延迟加载: 仅在首次调用 embed() 时实际加载。
        默认 all-MiniLM-L6-v2 模型约 80MB 内存。
        """
        if self._local_model is None:
            from sentence_transformers import SentenceTransformer

            logger.info("Loading local embedding model: %s", self.config.model)
            self._local_model = SentenceTransformer(self.config.model)
            logger.info("Model loaded. Dimension: %d", self.config.dimension)

    async def _ensure_client(self):
        """确保 HTTP 客户端已初始化 / Ensure HTTP client is initialized."""
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    def _cache_key(self, text: str) -> str:
        """
        生成缓存键 / Generate cache key.

        使用 MD5 哈希文本内容作为键, 避免存储原始文本。
        """
        return hashlib.md5(text.encode()).hexdigest()

    def _load_cache(self):
        """
        从磁盘加载嵌入缓存 / Load embedding cache from disk.

        缓存文件为 JSON 格式: {md5_hash: [float, float, ...]}
        仅在 cache_enabled=True 时启用。
        """
        if self.config.cache_enabled and self._cache_path.exists():
            try:
                data = json.loads(self._cache_path.read_text())
                self._cache = data
                logger.info("Loaded %d cached embeddings", len(self._cache))
            except Exception:
                self._cache = {}

    def _save_cache(self):
        """
        将嵌入缓存保存到磁盘 / Save embedding cache to disk.

        异步场景下可能存在竞争条件, 但 embedding 计算本身不频繁,
        简单的覆盖写策略足够。
        """
        if self.config.cache_enabled and self._cache:
            try:
                self._cache_path.write_text(json.dumps(self._cache))
            except Exception:
                pass

    async def embed(self, texts: List[str]) -> List[List[float]]:
        """
        批量嵌入文本 / Embed a list of texts into vectors.

        自动选择后端 (local/openai), 自动分批处理。
        返回的向量已归一化 (L2 norm = 1)。

        Args:
            texts: 要嵌入的文本列表 / Texts to embed

        Returns:
            List[List[float]]: 嵌入向量列表, 每个向量维度由模型决定

        Raises:
            ValueError: 未知嵌入提供商 / Unknown embedding provider
        """
        if not texts:
            return []

        if self.config.provider == "local":
            return await self._embed_local(texts)
        elif self.config.provider == "openai":
            return await self._embed_openai(texts)
        else:
            raise ValueError(f"Unknown embedding provider: {self.config.provider}")

    async def embed_single(self, text: str) -> List[float]:
        """
        嵌入单条文本 / Embed a single text.

        便捷方法, 内部调用 embed() 实现。

        Args:
            text: 要嵌入的文本

        Returns:
            List[float]: 嵌入向量
        """
        results = await self.embed([text])
        return results[0]

    async def _embed_local(self, texts: List[str]) -> List[List[float]]:
        """
        本地模型嵌入实现 / Local model embedding.

        使用 sentence-transformers 进行编码, 自动归一化。
        支持 GPU 加速 (如果可用)。

        Args:
            texts: 要嵌入的文本列表

        Returns:
            List[List[float]]: 嵌入向量列表
        """
        self._load_local_model()
        start = time.monotonic()

        # encode() 自动处理 batch_size 和归一化
        vectors = self._local_model.encode(
            texts,
            batch_size=self.config.batch_size,
            show_progress_bar=False,
            normalize_embeddings=True,  # L2 归一化确保 Cosine 距离正确
        )

        latency = (time.monotonic() - start) * 1000
        logger.info("Local embedding: %d texts in %.1fms", len(texts), latency)

        return [v.tolist() for v in vectors]

    async def _embed_openai(self, texts: List[str]) -> List[List[float]]:
        """
        OpenAI API 嵌入实现 / OpenAI API embedding.

        支持缓存 (默认启用) 以减少重复计费。
        自动分批处理以控制请求大小。

        Args:
            texts: 要嵌入的文本列表

        Returns:
            List[List[float]]: 嵌入向量列表
        """
        await self._ensure_client()

        # 检查缓存 / Check cache
        if self.config.cache_enabled:
            if not self._cache:
                self._load_cache()

            uncached = []
            uncached_indices = []
            results: List[Optional[List[float]]] = [None] * len(texts)

            for i, text in enumerate(texts):
                key = self._cache_key(text)
                if key in self._cache:
                    results[i] = self._cache[key]
                else:
                    uncached.append(text)
                    uncached_indices.append(i)

            if not uncached:
                return results  # type: ignore

            # 获取未缓存的嵌入 / Fetch uncached embeddings
            vectors = await self._fetch_openai_embeddings(uncached)
            for idx, vec in zip(uncached_indices, vectors):
                results[idx] = vec
                self._cache[self._cache_key(texts[idx])] = vec

            self._save_cache()
            return results  # type: ignore
        else:
            return await self._fetch_openai_embeddings(texts)

    async def _fetch_openai_embeddings(self, texts: List[str]) -> List[List[float]]:
        """
        向 OpenAI API 发送嵌入请求 / Send embedding request to OpenAI API.

        端点: POST /v1/embeddings
        自动按 batch_size 分批, 避免单次请求过大。

        Args:
            texts: 要嵌入的文本列表

        Returns:
            List[List[float]]: 嵌入向量列表
        """
        url = "https://api.openai.com/v1/embeddings"
        headers = {"Authorization": f"Bearer {self.config.openai_api_key}"}

        all_vectors = []
        batch_size = self.config.batch_size

        for i in range(0, len(texts), batch_size):
            batch = texts[i : i + batch_size]
            payload = {
                "model": self.config.openai_model,
                "input": batch,
            }

            start = time.monotonic()
            resp = await self._client.post(url, json=payload, headers=headers)
            latency = (time.monotonic() - start) * 1000
            resp.raise_for_status()
            data = resp.json()

            # OpenAI 返回顺序可能不固定, 按 index 排序
            sorted_data = sorted(data["data"], key=lambda x: x["index"])
            for item in sorted_data:
                all_vectors.append(item["embedding"])

            logger.info("OpenAI embedding: %d texts in %.1fms", len(batch), latency)

        return all_vectors

    async def close(self):
        """
        关闭服务, 释放资源 / Close service and release resources.

        应在应用关闭时调用。
        """
        if self._client and not self._client.is_closed:
            await self._client.aclose()
