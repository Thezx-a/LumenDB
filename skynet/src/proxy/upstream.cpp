#include "skynet/proxy/upstream.h"
namespace skynet {
namespace proxy {

void UpstreamManager::reload(const std::vector<Upstream>& upstreams) {
    std::lock_guard<std::mutex> lock(mutex_);
    upstreams_.clear();
    for (const auto& u : upstreams) {
        upstreams_.push_back(std::make_shared<Upstream>(u));
    }
    current_ = 0;
}

std::shared_ptr<Upstream> UpstreamManager::next() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < upstreams_.size(); ++i) {
        size_t idx = (current_ + i) % upstreams_.size();
        if (upstreams_[idx]->healthy) {
            current_ = (idx + 1) % upstreams_.size();
            return upstreams_[idx];
        }
    }
    return nullptr;
}

void UpstreamManager::markDown(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& u : upstreams_) {
        if (u->host == host && u->port == port) u->healthy = false;
    }
}

void UpstreamManager::markUp(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& u : upstreams_) {
        if (u->host == host && u->port == port) u->healthy = true;
    }
}

const std::vector<std::shared_ptr<Upstream>>& UpstreamManager::all() const {
    return upstreams_;
}

}  // namespace proxy
}  // namespace skynet
