#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <random>
#include <cmath>
#include <algorithm>

struct SearchResult {
    int64_t id;
    float distance;
};

struct VectorDBConfig {
    std::string data_path = "vectors.db";
    size_t dimension = 128;
    int64_t initial_capacity = 10000;
};

struct HNSWConfig {
    size_t M = 8;
    size_t M_max0 = 16;
    size_t ef_construction = 100;
    size_t dimension = 128;
    float mL = 0.36f;
};

class VectorStorage {
public:
    VectorStorage(const VectorDBConfig& config);
    ~VectorStorage();
    VectorStorage(const VectorStorage&) = delete;
    VectorStorage& operator=(const VectorStorage&) = delete;

    int64_t append(const float* vector);
    const float* get_vector(int64_t id) const;
    int64_t count() const;
    size_t dimension() const;
    void sync();

private:
    void resize_if_needed();
    int fd_ = -1;
    void* mmap_base_ = nullptr;
    size_t mmap_size_ = 0;
    size_t header_size_ = 28;
    int64_t* count_ptr_ = nullptr;
    float* data_base_ = nullptr;
    VectorDBConfig config_;
};

struct HNSWNode {
    int64_t id;
    int level;
    std::vector<std::vector<int64_t>> neighbors;
};

class HNSWIndex {
public:
    HNSWIndex(const HNSWConfig& config, VectorStorage* storage);
    void insert(int64_t id, const float* vector);
    std::vector<SearchResult> search(const float* query, int k, int ef = 0);

private:
    int random_level();
    float distance(const float* a, const float* b) const;
    void search_layer(const float* query, int64_t entry, int level, int ef,
                      std::vector<std::pair<int64_t, float>>& out);
    void select_neighbors(const float* query,
        const std::vector<std::pair<int64_t, float>>& in,
        size_t M_max, std::vector<int64_t>& out);

    HNSWConfig cfg_;
    VectorStorage* storage_;
    int64_t entry_point_ = 0;
    int max_level_ = -1;
    std::unordered_map<int64_t, HNSWNode> nodes_;
    std::mt19937 rng_{42};
    std::uniform_real_distribution<float> uni_{0.0f, 1.0f};
};

std::vector<SearchResult> brute_force_search(
    const VectorStorage& storage, const float* query, int k);
