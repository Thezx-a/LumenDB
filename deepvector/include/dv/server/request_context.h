#pragma once
#include <chrono>
#include <cstdint>
#include <string>

namespace dv {
namespace server {

/**
 * Per-request context propagated through HTTP handlers.
 * Mirrors Agent FastAPI X-Request-Id for cross-service tracing.
 */
struct RequestContext {
    std::string request_id;
    std::string collection = "default";
    std::string method;
    std::string path;
    int64_t start_ms = 0;

    static int64_t nowMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
                   steady_clock::now().time_since_epoch())
            .count();
    }

    int64_t elapsedMs() const { return nowMs() - start_ms; }
};

} // namespace server
} // namespace dv
