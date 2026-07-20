#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "lumendb/types.h"

namespace lumendb {
namespace storage {

class VectorStore {
public:
    VectorStore(Dimension dim, const std::string& path);
    ~VectorStore();

    uint64_t append(const float* vector);
    const float* get(uint64_t id) const;
    void remove(uint64_t id);  // soft delete, mark as removed
    size_t count() const { return count_; }
    Dimension dim() const { return dim_; }
    size_t capacity() const { return capacity_; }

    void flush();
    static std::unique_ptr<VectorStore> load(const std::string& path);

private:
    VectorStore(Dimension dim, int fd);  // For load()

    void ensureCapacity(size_t needed);
    void grow(size_t new_capacity);
    uint64_t nextID();

    Dimension dim_;
    std::string path_;
    int fd_;
    float* data_;       // mmap'd region
    uint64_t* ids_;     // id → offset mapping (mmap'd)
    size_t capacity_;
    size_t count_;
    size_t file_size_;
    bool dirty_;
};

} // namespace storage
} // namespace lumendb
