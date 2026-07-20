#pragma once
#include <cstddef>
#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include <unordered_map>
#include "skynet/net/socket.h"
#include "skynet/proxy/upstream.h"

namespace skynet {
namespace proxy {

class ConnectionPool {
public:
    explicit ConnectionPool(size_t max_idle = 10);
    std::unique_ptr<net::Socket> acquire(const Upstream& up);
    void release(std::unique_ptr<net::Socket> sock, const Upstream& up);

private:
    struct PoolEntry {
        std::queue<std::unique_ptr<net::Socket>> idle;
    };
    std::unordered_map<std::string, PoolEntry> pools_;
    std::mutex mutex_;
    size_t max_idle_;
};

}  // namespace proxy
}  // namespace skynet
