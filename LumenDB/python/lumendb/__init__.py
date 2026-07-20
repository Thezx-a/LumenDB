"""
LumenDB - C++ Zero-Copy Vector Database for RAG

Usage:
    import lumendb
    import numpy as np

    cfg = lumendb.CollectionConfig()
    cfg.dim = 768
    cfg.metric = lumendb.DistanceMetric.Cosine

    coll = lumendb.Collection(cfg, "/tmp/lumendb_py")
    vec = np.random.randn(768).astype(np.float32)

    coll.add(vec)
    results = coll.search(vec, k=10)
    for r in results:
        print(f"  id={r.id} dist={r.distance:.4f}")
"""

from lumendb._lumendb import (
    Collection,
    CollectionConfig,
    SearchResult,
    DocumentMeta,
    DistanceMetric,
    FilterNode,
)

__version__ = "0.1.0"
__all__ = [
    "Collection",
    "CollectionConfig",
    "SearchResult",
    "DocumentMeta",
    "DistanceMetric",
    "FilterNode",
]
