#pragma once
#include <cstddef>
#include <string>

namespace minikv {

struct Options {
    size_t memtable_size = 4 * 1024 * 1024;   // 4MB
    size_t block_size = 4 * 1024;              // 4KB
    size_t lru_cache_capacity = 8 * 1024 * 1024;  // 8MB
    int max_level = 7;
    size_t level0_compaction_trigger = 4;
    bool wal_sync = true;
    bool bloom_filter_enabled = true;
    double bloom_false_positive_rate = 0.01;
    std::string db_path;
};

struct ReadOptions {};
struct WriteOptions {
    bool sync = true;
};

}  // namespace minikv
