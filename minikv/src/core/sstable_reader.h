#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "core/bloom_filter.h"
#include "core/block.h"
#include "minikv/slice.h"
#include "minikv/status.h"

namespace minikv {
namespace core {

class SSTableReader {
public:
    static std::unique_ptr<SSTableReader> open(const std::string& path);
    std::optional<std::string> get(const Slice& key) const;
    bool mightContain(const Slice& key) const { return bloom_ && bloom_->mightContain(key); }
    Status scan(const Slice& start, const Slice& end,
                std::function<void(const Slice&, const Slice&)> callback) const;

    const std::string& path() const { return path_; }
    uint64_t fileSize() const { return file_size_; }

private:
    std::string path_;
    int fd_;
    uint64_t file_size_;
    uint64_t index_offset_;
    uint64_t index_size_;
    std::string index_data_;
    std::vector<std::pair<std::string, BlockHandle>> index_entries_;
    std::unique_ptr<BloomFilter> bloom_;
};

}  // namespace core
}  // namespace minikv
