#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "minikv/slice.h"

namespace minikv {
namespace core {

struct BlockHandle {
    uint64_t offset;
    uint64_t size;
};

class BlockBuilder {
public:
    explicit BlockBuilder(size_t block_size = 4096);
    void add(const Slice& key, const Slice& value);
    Slice finish();
    bool empty() const { return buffer_.empty(); }
    size_t size() const { return buffer_.size(); }
    size_t estimatedSize() const { return buffer_.size(); }

private:
    std::string buffer_;
    size_t block_size_;
    std::string last_key_;
    bool finished_;
};

class BlockReader {
public:
    explicit BlockReader(const Slice& block_data);
    std::optional<std::string> get(const Slice& key) const;
    size_t numEntries() const { return num_entries_; }

private:
    Slice data_;
    uint32_t num_entries_;
    uint32_t restarts_offset_;
    std::vector<uint32_t> restart_points_;
};

}  // namespace core
}  // namespace minikv
