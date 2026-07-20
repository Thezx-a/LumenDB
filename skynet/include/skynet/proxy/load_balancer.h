#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include "skynet/proxy/upstream.h"

namespace skynet {
namespace proxy {

class LoadBalancer {
public:
    virtual ~LoadBalancer() = default;
    virtual std::shared_ptr<Upstream> select() = 0;
    virtual void feedback(const Upstream& up, bool success) {}
};

class RoundRobinLB : public LoadBalancer {
public:
    explicit RoundRobinLB(UpstreamManager* mgr);
    std::shared_ptr<Upstream> select() override;
private:
    UpstreamManager* mgr_;
    size_t idx_{0};
};

class WeightedRoundRobinLB : public LoadBalancer {
public:
    explicit WeightedRoundRobinLB(UpstreamManager* mgr);
    std::shared_ptr<Upstream> select() override;
private:
    UpstreamManager* mgr_;
    std::vector<int> current_weights_;
};

class LeastConnLB : public LoadBalancer {
public:
    explicit LeastConnLB(UpstreamManager* mgr);
    std::shared_ptr<Upstream> select() override;
private:
    UpstreamManager* mgr_;
};

class ConsistentHashLB : public LoadBalancer {
public:
    explicit ConsistentHashLB(UpstreamManager* mgr, int vnodes = 160);
    std::shared_ptr<Upstream> select() override;
private:
    UpstreamManager* mgr_;
    std::vector<std::pair<uint32_t, std::shared_ptr<Upstream>>> ring_;
    void buildRing();
};

}  // namespace proxy
}  // namespace skynet
