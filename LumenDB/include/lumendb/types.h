#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace lumendb {

using VectorID = uint64_t;
using Dimension = uint32_t;
constexpr VectorID kInvalidID = 0;

enum class DistanceMetric : uint8_t {
    L2 = 0,
    InnerProduct = 1,
    Cosine = 2,
};

struct SearchResult {
    VectorID id;
    float distance;
};

struct CollectionConfig {
    Dimension dim = 0;
    DistanceMetric metric = DistanceMetric::L2;
    size_t hnsw_m = 16;                 // max neighbors per node
    size_t hnsw_ef_construction = 200;  // search width during construction
    size_t hnsw_ef_search = 50;         // search width during query
    size_t max_elements = 0;            // 0 = unlimited

    bool use_pq = false;
    size_t pq_M = 0;      // 0 = auto: dim/4
    size_t pq_K = 256;
    bool use_sq = false;
};

} // namespace lumendb
