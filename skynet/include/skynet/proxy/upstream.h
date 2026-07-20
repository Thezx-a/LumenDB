#pragma once
#include <cstddef>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace skynet {
namespace proxy {

struct Upstream {
    std::string host;
    int port;
    int weight;
    int active_connections{0};
    bool healthy{true};
};

class UpstreamManager {
public:
    void reload(const std::vector<Upstream>& upstreams);
    std::shared_ptr<Upstream> next();
    void markDown(const std::string& host, int port);
    void markUp(const std::string& host, int port);
    const std::vector<std::shared_ptr<Upstream>>& all() const;

private:
    std::vector<std::shared_ptr<Upstream>> upstreams_;
    std::mutex mutex_;
    size_t current_{0};
};

}  // namespace proxy
}  // namespace skynet
