#pragma once
#include <atomic>
#include <string>
#include "skynet/proxy/upstream.h"

namespace skynet {
namespace proxy {

class HealthCheck {
public:
    HealthCheck(UpstreamManager* mgr, int interval_s = 5, int timeout_ms = 200,
                int fail_threshold = 3, int recover_threshold = 2);
    void start();
    void stop();

private:
    UpstreamManager* mgr_;
    int interval_s_;
    int timeout_ms_;
    int fail_threshold_;
    int recover_threshold_;
    std::atomic<bool> running_{false};
};

}  // namespace proxy
}  // namespace skynet
