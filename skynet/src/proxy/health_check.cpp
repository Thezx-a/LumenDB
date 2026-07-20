#include "skynet/proxy/health_check.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <iostream>

namespace skynet {
namespace proxy {

HealthCheck::HealthCheck(UpstreamManager* mgr, int interval_s, int timeout_ms,
                          int fail_threshold, int recover_threshold)
    : mgr_(mgr), interval_s_(interval_s), timeout_ms_(timeout_ms),
      fail_threshold_(fail_threshold), recover_threshold_(recover_threshold) {}

void HealthCheck::start() {
    running_ = true;
    std::thread([this] {
        while (running_) {
            auto& all = const_cast<std::vector<std::shared_ptr<Upstream>>&>(mgr_->all());
            for (auto& up : all) {
                bool success = false;
                int fd = ::socket(AF_INET, SOCK_STREAM, 0);
                if (fd >= 0) {
                    struct sockaddr_in addr{};
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(up->port);
                    ::inet_pton(AF_INET, up->host.c_str(), &addr.sin_addr);
                    struct timeval tv{timeout_ms_ / 1000, (timeout_ms_ % 1000) * 1000};
                    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                    success = (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
                    ::close(fd);
                }
                if (success) {
                    mgr_->markUp(up->host, up->port);
                } else {
                    mgr_->markDown(up->host, up->port);
                }
            }
            ::sleep(interval_s_);
        }
    }).detach();
}

void HealthCheck::stop() { running_ = false; }

}  // namespace proxy
}  // namespace skynet
