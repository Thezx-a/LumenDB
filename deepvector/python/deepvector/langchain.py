"""
LangChain integration for DeepVector (langchain-core APIs).

Usage:
    from deepvector.langchain import DeepVectorVectorStore
    from langchain_openai import OpenAIEmbeddings  # optional

    embeddings = OpenAIEmbeddings()
    store = DeepVectorVectorStore(embedding=embeddings, config=cfg)
    store.add_texts(["hello world", "goodbye world"])
    results = store.similarity_search("hello", k=5)
"""

from __future__ import annotations

from typing import Any, Iterable, List, Optional, Tuple
import numpy as np

try:
    from langchain_core.documents import Document
    from langchain_core.vectorstores import VectorStore
except ImportError:  # pragma: no cover - optional dependency
    try:
        from langchain_core.documents import Document  # type: ignore
        from langchain_core.vectorstores.base import VectorStore  # type: ignore
    except ImportError:
        Document = None  # type: ignore
        VectorStore = object  # type: ignore


class DeepVectorVectorStore(VectorStore if VectorStore is not object else object):
    """LangChain-compatible vector store backed by DeepVector C++ bindings."""

    def __init__(
        self,
        embedding: Any,
        config: Any = None,
        data_dir: str = "/tmp/deepvector_langchain",
        **kwargs: Any,
    ):
        from deepvector._deepvector import Collection, CollectionConfig, DistanceMetric

        self._embedding = embedding
        self._data_dir = data_dir

        if config is None:
            config = CollectionConfig()
            config.dim = 384
            config.metric = DistanceMetric.Cosine
        self._coll = Collection(config, data_dir)
        self._config = config

    @property
    def embeddings(self) -> Any:
        return self._embedding

    def add_texts(
        self,
        texts: Iterable[str],
        metadatas: Optional[List[dict]] = None,
        **kwargs: Any,
    ) -> List[str]:
        from deepvector._deepvector import DocumentMeta
        import time

        text_list = list(texts)
        ids: List[str] = []
        vectors = self._embedding.embed_documents(text_list)
        for i, (text, vec) in enumerate(zip(text_list, vectors)):
            arr = np.array(vec, dtype=np.float32)
            meta = DocumentMeta()
            meta.text = text
            meta.timestamp = int(time.time())
            if metadatas and i < len(metadatas):
                meta.tags = ",".join(f"{k}:{v}" for k, v in metadatas[i].items())
            vid = self._coll.add_with_meta(arr, meta)
            ids.append(str(vid))
        return ids

    def similarity_search(
        self, query: str, k: int = 4, **kwargs: Any
    ) -> List[Any]:
        vec = np.array(self._embedding.embed_query(query), dtype=np.float32)
        results = self._coll.search(vec, k)
        docs: List[Any] = []
        for r in results:
            meta = self._coll.get_meta(r.id)
            if not meta:
                continue
            metadata = {"id": r.id, "distance": r.distance}
            if Document is not None:
                docs.append(Document(page_content=meta.text, metadata=metadata))
            else:
                docs.append({"page_content": meta.text, "metadata": metadata})
        return docs

    def similarity_search_with_score(
        self, query: str, k: int = 4, **kwargs: Any
    ) -> List[Tuple[Any, float]]:
        vec = np.array(self._embedding.embed_query(query), dtype=np.float32)
        results = self._coll.search(vec, k)
        out: List[Tuple[Any, float]] = []
        for r in results:
            meta = self._coll.get_meta(r.id)
            if not meta:
                continue
            metadata = {"id": r.id, "distance": r.distance}
            doc = (
                Document(page_content=meta.text, metadata=metadata)
                if Document is not None
                else {"page_content": meta.text, "metadata": metadata}
            )
            out.append((doc, float(r.distance)))
        return out

    @classmethod
    def from_texts(
        cls,
        texts: List[str],
        embedding: Any,
        metadatas: Optional[List[dict]] = None,
        **kwargs: Any,
    ):
        instance = cls(embedding=embedding, **kwargs)
        instance.add_texts(texts, metadatas)
        return instance

    def _similarity_search_with_relevance_scores(
        self, query: str, k: int = 4, **kwargs: Any
    ) -> List[Tuple[Any, float]]:
        return self.similarity_search_with_score(query, k=k, **kwargs)
