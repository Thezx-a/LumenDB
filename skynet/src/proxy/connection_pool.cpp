#include "skynet/proxy/connection_pool.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace skynet {
namespace proxy {

ConnectionPool::ConnectionPool(size_t max_idle) : max_idle_(max_idle) {}

std::unique_ptr<net::Socket> ConnectionPool::acquire(const Upstream& up) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = up.host + ":" + std::to_string(up.port);
    auto it = pools_.find(key);
    if (it != pools_.end() && !it->second.idle.empty()) {
        auto sock = std::move(it->second.idle.front());
        it->second.idle.pop();
        return sock;
    }
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return nullptr;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(up.port);
    ::inet_pton(AF_INET, up.host.c_str(), &addr.sin_addr);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd); return nullptr;
    }
    return std::make_unique<net::Socket>(fd);
}

void ConnectionPool::release(std::unique_ptr<net::Socket> sock, const Upstream& up) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = up.host + ":" + std::to_string(up.port);
    auto& pool = pools_[key];
    if (pool.idle.size() < max_idle_) {
        pool.idle.push(std::move(sock));
    }
}

}  // namespace proxy
}  // namespace skynet
