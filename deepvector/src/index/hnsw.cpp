#include "lumendb/index/hnsw.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace dv {
namespace index {

HNSWIndex::HNSWIndex(size_t M, size_t ef_construction)
    : M_(M), ef_construction_(std::max(ef_construction, M)),
      ef_search_(ef_construction), max_level_(-1), entry_point_(0),
      element_count_(0), mult_(1.0 / std::log(1.0 * M)) {
    M_max_ = M_;
    M_max0_ = M_ * 2;
    std::random_device rd;
    rng_.seed(rd());
    nodes_.reserve(1024);
}

int HNSWIndex::randomLevel() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = -std::log(dist(rng_)) * mult_;
    return static_cast<int>(r);
}

void HNSWIndex::selectNeighborsSimple(const std::vector<DistId>& candidates,
                                       size_t M, std::vector<DistId>& result) {
    result = candidates;
    std::sort(result.begin(), result.end());
    if (result.size() > M) result.resize(M);
}

void HNSWIndex::pruneNeighbors(int lc, uint64_t nodeId, size_t maxConn) {
    auto& neighbors = nodes_[nodeId].neighbors[lc];
    if (neighbors.size() <= maxConn) return;

    std::vector<DistId> neighborDists;
    for (uint64_t nid : neighbors) {
        neighborDists.push_back({pairwise_dist_(nodeId, nid), nid});
    }
    std::sort(neighborDists.begin(), neighborDists.end());
    neighborDists.resize(maxConn);

    neighbors.clear();
    for (auto& nd : neighborDists) {
        neighbors.push_back(nd.second);
    }
}

std::vector<uint64_t> HNSWIndex::searchLayer(const float* query, uint64_t entry, int lc, size_t ef) const {
    auto cmpMin = [](const DistId& a, const DistId& b) { return a.first < b.first; };
    auto cmpMax = [](const DistId& a, const DistId& b) { return a.first > b.first; };

    std::priority_queue<DistId, std::vector<DistId>, decltype(cmpMax)> candidates(cmpMax);
    std::priority_queue<DistId, std::vector<DistId>, decltype(cmpMin)> topEf(cmpMin);

    float entryDist = query_dist_(entry, query);
    candidates.push({entryDist, entry});
    topEf.push({entryDist, entry});

    std::unordered_set<uint64_t> visited;
    visited.insert(entry);

    while (!candidates.empty()) {
        auto [candDist, candId] = candidates.top();
        candidates.pop();

        if (candDist > topEf.top().first) break;

        if (candId >= nodes_.size() || lc >= static_cast<int>(nodes_[candId].neighbors.size())) continue;

        for (uint64_t neighborId : nodes_[candId].neighbors[lc]) {
            if (visited.count(neighborId)) continue;
            visited.insert(neighborId);

            if (neighborId >= nodes_.size()) continue;

            float dist = query_dist_(neighborId, query);

            if (topEf.size() < ef || dist < topEf.top().first) {
                candidates.push({dist, neighborId});
                topEf.push({dist, neighborId});
                if (topEf.size() > ef) topEf.pop();
            }
        }
    }

    std::vector<uint64_t> result;
    result.reserve(topEf.size());
    while (!topEf.empty()) {
        result.push_back(topEf.top().second);
        topEf.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void HNSWIndex::insert(uint64_t id) {
    if (!pairwise_dist_ || !query_dist_) return;

    int level = randomLevel();
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (id >= nodes_.size()) nodes_.resize(id + 1);
    nodes_[id] = HNSWNode();
    nodes_[id].id = id;
    nodes_[id].level = level;
    nodes_[id].neighbors.resize(level + 1);
    nodes_[id].deleted = false;

    if (element_count_ == 0) {
        entry_point_ = id;
        max_level_ = level;
    } else {
        auto insertQuery = [this, id](uint64_t other, const float*) -> float {
            return pairwise_dist_(id, other);
        };
        auto savedQuery = query_dist_;
        query_dist_ = insertQuery;

        uint64_t curr = entry_point_;
        int currLevel = max_level_;

        for (int lc = currLevel; lc > level; --lc) {
            auto candidates = searchLayer(nullptr, curr, lc, 1);
            if (!candidates.empty()) curr = candidates[0];
        }

        for (int lc = std::min(level, max_level_); lc >= 0; --lc) {
            auto candidates = searchLayer(nullptr, curr, lc, ef_construction_);

            std::vector<DistId> candDist;
            for (uint64_t nid : candidates) {
                candDist.push_back({pairwise_dist_(id, nid), nid});
            }
            std::vector<DistId> selected;
            selectNeighborsSimple(candDist, M_, selected);

            for (auto& s : selected) {
                nodes_[id].neighbors[lc].push_back(s.second);
                if (s.second < nodes_.size() && lc < static_cast<int>(nodes_[s.second].neighbors.size())) {
                    nodes_[s.second].neighbors[lc].push_back(id);
                }
            }

            if (!candidates.empty()) curr = candidates[0];
        }

        query_dist_ = savedQuery;
    }

    if (level > max_level_) {
        max_level_ = level;
        entry_point_ = id;
    }

    for (int lc = 0; lc <= level; ++lc) {
        size_t maxConn = (lc == 0) ? M_max0_ : M_max_;
        for (uint64_t nid : nodes_[id].neighbors[lc]) {
            if (nid < nodes_.size() && lc < static_cast<int>(nodes_[nid].neighbors.size())) {
                pruneNeighbors(lc, nid, maxConn);
            }
        }
        pruneNeighbors(lc, id, maxConn);
    }

    element_count_++;
}

void HNSWIndex::remove(uint64_t id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (id < nodes_.size() && !nodes_[id].deleted) {
        nodes_[id].deleted = true;
        if (element_count_ > 0) element_count_--;
    }
}

std::vector<SearchResult> HNSWIndex::search(const float* query, size_t k) const {
    if (element_count_ == 0 || !query_dist_) return {};

    std::shared_lock<std::shared_mutex> lock(mutex_);

    uint64_t curr = entry_point_;
    for (int lc = max_level_; lc > 0; --lc) {
        auto candidates = searchLayer(query, curr, lc, 1);
        if (!candidates.empty()) curr = candidates[0];
    }

    auto candidateIds = searchLayer(query, curr, 0, ef_search_);

    std::vector<DistId> candidates;
    for (uint64_t cid : candidateIds) {
        if (cid < nodes_.size() && !nodes_[cid].deleted) {
            float dist = query_dist_(cid, query);
            candidates.push_back({dist, cid});
        }
    }
    std::sort(candidates.begin(), candidates.end());
    if (candidates.size() > k) candidates.resize(k);

    std::vector<SearchResult> results;
    for (auto& [dist, cid] : candidates) {
        results.push_back({cid, dist});
    }
    return results;
}

} // namespace index
} // namespace dv
