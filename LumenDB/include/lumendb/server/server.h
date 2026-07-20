#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <vector>
#include "lumendb/types.h"

namespace lumendb {
class Collection;

namespace server {

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    size_t num_threads = 4;
    size_t max_connections = 10000;
    std::string data_dir = "./lumendb_data";
    std::string api_key = "";
};

struct ServerStats {
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> search_requests{0};
    std::atomic<uint64_t> insert_requests{0};
    std::atomic<uint64_t> error_requests{0};
    std::atomic<uint64_t> active_connections{0};
};

class LumenDBServer {
public:
    explicit LumenDBServer(const ServerConfig& config, std::unique_ptr<Collection> collection);
    ~LumenDBServer();

    void start();
    void stop();
    const ServerStats& stats() const { return stats_; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    ServerConfig config_;
    ServerStats stats_;
    std::unique_ptr<Collection> collection_;
};

} // namespace server
} // namespace lumendb
