#include "skynet/proxy/load_balancer.h"
#include <algorithm>
#include <cstdint>
#include <functional>

namespace skynet {
namespace proxy {

// --- RoundRobinLB ---
RoundRobinLB::RoundRobinLB(UpstreamManager* mgr) : mgr_(mgr) {}
std::shared_ptr<Upstream> RoundRobinLB::select() {
    auto& all = const_cast<std::vector<std::shared_ptr<Upstream>>&>(mgr_->all());
    if (all.empty()) return nullptr;
    for (size_t i = 0; i < all.size(); ++i) {
        size_t idx = (idx_ + i) % all.size();
        if (all[idx]->healthy) { idx_ = (idx + 1) % all.size(); return all[idx]; }
    }
    return nullptr;
}

// --- WeightedRoundRobinLB (Nginx SWRR) ---
WeightedRoundRobinLB::WeightedRoundRobinLB(UpstreamManager* mgr)
    : mgr_(mgr), current_weights_() {
    auto& all = const_cast<std::vector<std::shared_ptr<Upstream>>&>(mgr_->all());
    current_weights_.resize(all.size(), 0);
}

std::shared_ptr<Upstream> WeightedRoundRobinLB::select() {
    auto& all = const_cast<std::vector<std::shared_ptr<Upstream>>&>(mgr_->all());
    if (all.empty()) return nullptr;
    if (current_weights_.size() != all.size())
        current_weights_.resize(all.size(), 0);

    int totalWeight = 0;
    int bestIdx = -1;
    int bestWeight = -1;
    for (size_t i = 0; i < all.size(); ++i) {
        if (!all[i]->healthy) continue;
        current_weights_[i] += all[i]->weight;
        totalWeight += all[i]->weight;
        if (current_weights_[i] > bestWeight) {
            bestWeight = current_weights_[i];
            bestIdx = static_cast<int>(i);
        }
    }
    if (bestIdx < 0) return nullptr;
    current_weights_[bestIdx] -= totalWeight;
    return all[bestIdx];
}

// --- LeastConnLB ---
LeastConnLB::LeastConnLB(UpstreamManager* mgr) : mgr_(mgr) {}
std::shared_ptr<Upstream> LeastConnLB::select() {
    auto& all = const_cast<std::vector<std::shared_ptr<Upstream>>&>(mgr_->all());
    if (all.empty()) return nullptr;
    std::shared_ptr<Upstream> best = nullptr;
    int minConn = INT32_MAX;
    for (auto& u : all) {
        if (!u->healthy) continue;
        if (u->active_connections < minConn) { minConn = u->active_connections; best = u; }
    }
    return best;
}

// --- ConsistentHashLB ---
ConsistentHashLB::ConsistentHashLB(UpstreamManager* mgr, int vnodes)
    : mgr_(mgr), ring_() { buildRing(); }

void ConsistentHashLB::buildRing() {
    ring_.clear();
    auto& all = const_cast<std::vector<std::shared_ptr<Upstream>>&>(mgr_->all());
    for (auto& u : all) {
        for (int i = 0; i < 160; ++i) {
            std::string key = u->host + ":" + std::to_string(u->port) + ":" + std::to_string(i);
            uint32_t h = std::hash<std::string>{}(key);
            ring_.push_back({h, u});
        }
    }
    std::sort(ring_.begin(), ring_.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
}

std::shared_ptr<Upstream> ConsistentHashLB::select() {
    if (ring_.empty()) return nullptr;
    uint32_t h = std::hash<std::string>{}("default");
    auto it = std::lower_bound(ring_.begin(), ring_.end(), h,
        [](const auto& entry, uint32_t v) { return entry.first < v; });
    if (it == ring_.end()) it = ring_.begin();
    return it->second;
}

}  // namespace proxy
}  // namespace skynet
