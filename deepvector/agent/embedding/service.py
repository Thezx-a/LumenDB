"""
Embedding Service 鈥?鏂囨湰鍚戦噺鍖栫殑缁熶竴鎺ュ彛 / Unified text embedding interface.

璁捐鍘熷垯 / Design Principles:
  1. 澶氬悗绔敮鎸? 鏈湴 sentence-transformers 鍜?OpenAI API
  2. 寤惰繜鍔犺浇: 鏈湴妯″瀷鍦ㄩ娆′娇鐢ㄦ椂鎵嶅姞杞?/ Lazy model loading
  3. 缂撳瓨: 鍙€?MD5 缂撳瓨, 閬垮厤閲嶅璁＄畻 / Optional MD5-based caching
  4. 鎵归噺澶勭悊: 鑷姩鍒嗘壒浠ュ噺灏?API 璋冪敤 / Auto-batching for API efficiency

渚涘簲鍟嗗樊寮?/ Provider Differences:
  - 鏈湴: 鍏嶈垂, 鏃犵綉缁滀緷璧? 384缁?/ Free, no network dependency, 384-d
  - OpenAI: 鎸夐噺浠樿垂, 鏇撮珮绮惧害, 1536缁?/ Pay-per-use, higher quality, 1536-d
  
鎬ц兘璇存槑 / Performance Notes:
  - 鏈湴 all-MiniLM-L6-v2: ~100 text/s (CPU), ~500 text/s (GPU)
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
    鏂囨湰宓屽叆鏈嶅姟 / Text Embedding Service.

    鏀寔鏈湴 sentence-transformers 鍜?OpenAI API 涓ょ妯″紡銆?    鎻愪緵 embed() 鎵归噺鎺ュ彛鍜?embed_single() 鍗曟潯鎺ュ彛銆?
    鐢ㄦ硶 / Usage:
        config = EmbeddingConfig(provider="local")
        svc = EmbeddingService(config)

        # 鎵归噺宓屽叆 / Batch embedding
        vectors = await svc.embed(["Hello world", "Another text"])
        print(len(vectors[0]))  # 384 for all-MiniLM-L6-v2

        # 鍗曟潯宓屽叆 / Single text
        vec = await svc.embed_single("What is RAG?")
    """

    def __init__(self, config: EmbeddingConfig):
        """
        鍒濆鍖栧祵鍏ユ湇鍔?/ Initialize embedding service.

        Args:
            config: 宓屽叆閰嶇疆, 鍖呭惈 provider/model/keys 绛?        """
        self.config = config
        self._local_model: Any = None  # 寤惰繜鍔犺浇 / Lazy load
        self._client: Optional[httpx.AsyncClient] = None
        self._cache: Dict[str, List[float]] = {}
        self._cache_path = Path(".embedding_cache.json")

    def _load_local_model(self):
        """
        鍔犺浇鏈湴 sentence-transformers 妯″瀷 / Load local model.

        寤惰繜鍔犺浇: 浠呭湪棣栨璋冪敤 embed() 鏃跺疄闄呭姞杞姐€?        榛樿 all-MiniLM-L6-v2 妯″瀷绾?80MB 鍐呭瓨銆?        """
        if self._local_model is None:
            from sentence_transformers import SentenceTransformer

            logger.info("Loading local embedding model: %s", self.config.model)
            self._local_model = SentenceTransformer(self.config.model)
            logger.info("Model loaded. Dimension: %d", self.config.dimension)

    async def _ensure_client(self):
        """纭繚 HTTP 瀹㈡埛绔凡鍒濆鍖?/ Ensure HTTP client is initialized."""
        if self._client is None or self._client.is_closed:
            self._client = httpx.AsyncClient(timeout=30.0)

    def _cache_key(self, text: str) -> str:
        """
        鐢熸垚缂撳瓨閿?/ Generate cache key.

        浣跨敤 MD5 鍝堝笇鏂囨湰鍐呭浣滀负閿? 閬垮厤瀛樺偍鍘熷鏂囨湰銆?        """
        return hashlib.md5(text.encode()).hexdigest()

    def _load_cache(self):
        """
        浠庣鐩樺姞杞藉祵鍏ョ紦瀛?/ Load embedding cache from disk.

        缂撳瓨鏂囦欢涓?JSON 鏍煎紡: {md5_hash: [float, float, ...]}
        浠呭湪 cache_enabled=True 鏃跺惎鐢ㄣ€?        """
        if self.config.cache_enabled and self._cache_path.exists():
            try:
                data = json.loads(self._cache_path.read_text())
                self._cache = data
                logger.info("Loaded %d cached embeddings", len(self._cache))
            except Exception:
                self._cache = {}

    def _save_cache(self):
        """
        灏嗗祵鍏ョ紦瀛樹繚瀛樺埌纾佺洏 / Save embedding cache to disk.

        寮傛鍦烘櫙涓嬪彲鑳藉瓨鍦ㄧ珵浜夋潯浠? 浣?embedding 璁＄畻鏈韩涓嶉绻?
        绠€鍗曠殑瑕嗙洊鍐欑瓥鐣ヨ冻澶熴€?        """
        if self.config.cache_enabled and self._cache:
            try:
                self._cache_path.write_text(json.dumps(self._cache))
            except Exception:
                pass

    async def embed(self, texts: List[str]) -> List[List[float]]:
        """
        鎵归噺宓屽叆鏂囨湰 / Embed a list of texts into vectors.

        鑷姩閫夋嫨鍚庣 (local/openai), 鑷姩鍒嗘壒澶勭悊銆?        杩斿洖鐨勫悜閲忓凡褰掍竴鍖?(L2 norm = 1)銆?
        Args:
            texts: 瑕佸祵鍏ョ殑鏂囨湰鍒楄〃 / Texts to embed

        Returns:
            List[List[float]]: 宓屽叆鍚戦噺鍒楄〃, 姣忎釜鍚戦噺缁村害鐢辨ā鍨嬪喅瀹?
        Raises:
            ValueError: 鏈煡宓屽叆鎻愪緵鍟?/ Unknown embedding provider
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
        宓屽叆鍗曟潯鏂囨湰 / Embed a single text.

        渚挎嵎鏂规硶, 鍐呴儴璋冪敤 embed() 瀹炵幇銆?
        Args:
            text: 瑕佸祵鍏ョ殑鏂囨湰

        Returns:
            List[float]: 宓屽叆鍚戦噺
        """
        results = await self.embed([text])
        return results[0]

    async def _embed_local(self, texts: List[str]) -> List[List[float]]:
        """
        鏈湴妯″瀷宓屽叆瀹炵幇 / Local model embedding.

        浣跨敤 sentence-transformers 杩涜缂栫爜, 鑷姩褰掍竴鍖栥€?        鏀寔 GPU 鍔犻€?(濡傛灉鍙敤)銆?
        Args:
            texts: 瑕佸祵鍏ョ殑鏂囨湰鍒楄〃

        Returns:
            List[List[float]]: 宓屽叆鍚戦噺鍒楄〃
        """
        self._load_local_model()
        start = time.monotonic()

        # encode() handles batch_size and normalization internally
        vectors = self._local_model.encode(
            texts,
            batch_size=self.config.batch_size,
            show_progress_bar=False,
            normalize_embeddings=True,  # L2 褰掍竴鍖栫‘淇?Cosine 璺濈姝ｇ‘
        )

        latency = (time.monotonic() - start) * 1000
        logger.info("Local embedding: %d texts in %.1fms", len(texts), latency)

        return [v.tolist() for v in vectors]

    async def _embed_openai(self, texts: List[str]) -> List[List[float]]:
        """
        OpenAI API 宓屽叆瀹炵幇 / OpenAI API embedding.

        鏀寔缂撳瓨 (榛樿鍚敤) 浠ュ噺灏戦噸澶嶈璐广€?        鑷姩鍒嗘壒澶勭悊浠ユ帶鍒惰姹傚ぇ灏忋€?
        Args:
            texts: 瑕佸祵鍏ョ殑鏂囨湰鍒楄〃

        Returns:
            List[List[float]]: 宓屽叆鍚戦噺鍒楄〃
        """
        await self._ensure_client()

        # 妫€鏌ョ紦瀛?/ Check cache
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

            # 鑾峰彇鏈紦瀛樼殑宓屽叆 / Fetch uncached embeddings
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
        鍚?OpenAI API 鍙戦€佸祵鍏ヨ姹?/ Send embedding request to OpenAI API.

        绔偣: POST /v1/embeddings
        鑷姩鎸?batch_size 鍒嗘壒, 閬垮厤鍗曟璇锋眰杩囧ぇ銆?
        Args:
            texts: 瑕佸祵鍏ョ殑鏂囨湰鍒楄〃

        Returns:
            List[List[float]]: 宓屽叆鍚戦噺鍒楄〃
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

            # OpenAI 杩斿洖椤哄簭鍙兘涓嶅浐瀹? 鎸?index 鎺掑簭
            sorted_data = sorted(data["data"], key=lambda x: x["index"])
            for item in sorted_data:
                all_vectors.append(item["embedding"])

            logger.info("OpenAI embedding: %d texts in %.1fms", len(batch), latency)

        return all_vectors

    async def close(self):
        """
        鍏抽棴鏈嶅姟, 閲婃斁璧勬簮 / Close service and release resources.

        搴斿湪搴旂敤鍏抽棴鏃惰皟鐢ㄣ€?        """
        if self._client and not self._client.is_closed:
            await self._client.aclose()
