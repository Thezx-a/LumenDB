"""
LangChain integration for LumenDB.

Usage:
    from lumendb.langchain import LumenDBVectorStore
    from langchain.embeddings import OpenAIEmbeddings

    embeddings = OpenAIEmbeddings()
    store = LumenDBVectorStore(embedding=embeddings, config=cfg)

    store.add_texts(["hello world", "goodbye world"])
    results = store.similarity_search("hello", k=5)
"""

from typing import Any, List, Optional
import numpy as np

try:
    from langchain.schema import Document
    from langchain.vectorstores.base import VectorStore
except ImportError:
    Document = None
    VectorStore = object


class LumenDBVectorStore(VectorStore if VectorStore is not object else object):
    """LangChain-compatible vector store backed by LumenDB."""

    def __init__(
        self,
        embedding: Any,
        config: Any = None,
        data_dir: str = "/tmp/lumendb_langchain",
        **kwargs: Any,
    ):
        from lumendb._lumendb import Collection, CollectionConfig

        self._embedding = embedding
        self._data_dir = data_dir

        if config is None:
            config = CollectionConfig()
            config.dim = 768
            config.metric = 2  # Cosine
        self._coll = Collection(config, data_dir)
        self._config = config

    @property
    def embeddings(self) -> Any:
        return self._embedding

    def add_texts(
        self,
        texts: List[str],
        metadatas: Optional[List[dict]] = None,
        **kwargs: Any,
    ) -> List[str]:
        from lumendb._lumendb import DocumentMeta
        import time

        ids = []
        vectors = self._embedding.embed_documents(texts)
        for i, (text, vec) in enumerate(zip(texts, vectors)):
            arr = np.array(vec, dtype=np.float32)
            meta = DocumentMeta()
            meta.text = text
            meta.timestamp = int(time.time())
            if metadatas and i < len(metadatas):
                meta.tags = ",".join(
                    f"{k}:{v}" for k, v in metadatas[i].items()
                )
            vid = self._coll.add_with_meta(arr, meta)
            ids.append(str(vid))
        return ids

    def similarity_search(
        self, query: str, k: int = 4, **kwargs: Any
    ) -> Any:
        vec = np.array(self._embedding.embed_query(query), dtype=np.float32)
        results = self._coll.search(vec, k)
        docs = []
        for r in results:
            meta = self._coll.get_meta(r.id)
            if meta:
                page_content = meta.text
                metadata = {"id": r.id, "distance": r.distance}
                if Document:
                    docs.append(Document(page_content=page_content, metadata=metadata))
                else:
                    docs.append({"page_content": page_content, "metadata": metadata})
        return docs

    def similarity_search_with_filter(
        self, query: str, k: int, filter_expr: Any, **kwargs: Any
    ) -> Any:
        vec = np.array(self._embedding.embed_query(query), dtype=np.float32)
        results = self._coll.search_with_filter(vec, k, filter_expr)
        docs = []
        for r in results:
            meta = self._coll.get_meta(r.id)
            if meta:
                if Document:
                    docs.append(Document(page_content=meta.text, metadata={"id": r.id}))
                else:
                    docs.append({"page_content": meta.text, "metadata": {"id": r.id}})
        return docs

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
