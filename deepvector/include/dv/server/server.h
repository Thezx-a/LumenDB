#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <atomic>
#include "dv/types.h"
#include "dv/collection.h"
#include "dv/server/collection_registry.h"
#include "dv/server/request_context.h"

namespace dv {
namespace server {

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    size_t num_threads = 4;
    size_t max_connections = 10000;
    std::string data_dir = "./deepvector_data";
    std::string api_key = "";
    std::string default_collection = "default";
};

struct ServerStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> search_requests{0};
    std::atomic<uint64_t> insert_requests{0};
    std::atomic<uint64_t> error_requests{0};
    std::atomic<uint64_t> active_connections{0};
    std::atomic<uint64_t> search_latency_sum_us{0};
    std::atomic<uint64_t> insert_latency_sum_us{0};
};

class DeepVectorServer {
public:
    explicit DeepVectorServer(const ServerConfig& config, const CollectionConfig& coll_config);
    ~DeepVectorServer();

    void start();
    void stop();
    const ServerStats& stats() const { return stats_; }
    CollectionRegistry& registry() { return *registry_; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    ServerConfig config_;
    ServerStats stats_;
    std::unique_ptr<CollectionRegistry> registry_;
};

} // namespace server
} // namespace dv
