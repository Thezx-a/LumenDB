#pragma once

#include <cstdint>
#include <string>
#include <vector>

class VectorStorage {
public:
    VectorStorage(size_t dimension, size_t capacity = 10000);
    ~VectorStorage() = default;

    VectorStorage(const VectorStorage&) = delete;
    VectorStorage& operator=(const VectorStorage&) = delete;

    int64_t append(const float* vector);
    const float* get_vector(int64_t id) const;
    int64_t count() const;
    size_t dimension() const;

    void save(const std::string& path) const;
    static VectorStorage load(const std::string& path);

private:
    size_t dimension_;
    int64_t count_ = 0;
    std::vector<float> data_;
};
