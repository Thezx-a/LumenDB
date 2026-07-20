#pragma once
#include <cstdint>
#include <functional>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <vector>
#include <queue>
#include "lumendb/types.h"

namespace lumendb {
namespace index {

using PairwiseDistance = std::function<float(uint64_t, uint64_t)>;
using QueryDistance = std::function<float(uint64_t, const float*)>;

struct HNSWNode {
    uint64_t id;
    int level;
    std::vector<std::vector<uint64_t>> neighbors;
    bool deleted;
    HNSWNode() : id(0), level(-1), deleted(false) {}
};

using DistId = std::pair<float, uint64_t>;

class HNSWIndex {
public:
    explicit HNSWIndex(size_t M = 16, size_t ef_construction = 200);

    void setPairwiseDistance(PairwiseDistance f) { pairwise_dist_ = std::move(f); }
    void setQueryDistance(QueryDistance f) { query_dist_ = std::move(f); }

    void setEfSearch(size_t ef) { ef_search_ = ef; }
    size_t getEfSearch() const { return ef_search_; }

    void insert(uint64_t id);
    void remove(uint64_t id);
    std::vector<SearchResult> search(const float* query, size_t k) const;
    size_t size() const { return element_count_; }
    bool empty() const { return element_count_ == 0; }

private:
    int randomLevel();
    std::vector<uint64_t> searchLayer(const float* query, uint64_t entry, int lc, size_t ef) const;
    void selectNeighborsSimple(const std::vector<DistId>& candidates, size_t M, std::vector<DistId>& result);
    void pruneNeighbors(int level, uint64_t nodeId, size_t maxConn);

    size_t M_;
    size_t M_max_;
    size_t M_max0_;
    size_t ef_construction_;
    mutable size_t ef_search_;
    int max_level_;
    uint64_t entry_point_;
    size_t element_count_;

    PairwiseDistance pairwise_dist_;
    QueryDistance query_dist_;

    std::vector<HNSWNode> nodes_;
    double mult_;
    mutable std::mt19937 rng_;
    mutable std::shared_mutex mutex_;
};

} // namespace index
} // namespace lumendb
