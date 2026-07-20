"""
DeepVector - C++ Zero-Copy Vector Database for RAG

Usage:
    import deepvector
    import numpy as np

    cfg = deepvector.CollectionConfig()
    cfg.dim = 768
    cfg.metric = deepvector.DistanceMetric.Cosine

    coll = deepvector.Collection(cfg, "/tmp/deepvector_py")
    vec = np.random.randn(768).astype(np.float32)

    coll.add(vec)
    results = coll.search(vec, k=10)
    for r in results:
        print(f"  id={r.id} dist={r.distance:.4f}")
"""

from deepvector._deepvector import (
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
