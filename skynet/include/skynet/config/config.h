#pragma once
#include <string>
#include <vector>
#include <memory>

namespace skynet {
namespace config {

struct UpstreamConfig {
    std::string host;
    int port;
    int weight;
};

struct HealthCheckConfig {
    int interval_s = 5;
    int timeout_ms = 200;
    std::string path = "/health";
};

struct LimitsConfig {
    int max_connections = 10000;
    int per_ip_max = 100;
};

struct Config {
    int listen_port = 8080;
    int worker_threads = 4;
    std::vector<UpstreamConfig> upstreams;
    HealthCheckConfig health_check;
    LimitsConfig limits;
    static std::unique_ptr<Config> load(const std::string& path);
};

}  // namespace config
}  // namespace skynet
