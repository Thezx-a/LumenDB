#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "core/block.h"
#include "core/bloom_filter.h"
#include "minikv/slice.h"
#include "minikv/status.h"

namespace minikv {
namespace core {

class SSTableBuilder {
public:
    SSTableBuilder(const std::string& path, size_t block_size = 4096);
    ~SSTableBuilder();
    Status add(uint64_t internalKey, const Slice& key, const Slice& value);
    Status finish();
    uint64_t fileSize() const;

private:
    Status flushDataBlock();
    void writeFooter();

    std::string path_;
    int fd_;
    size_t block_size_;
    BlockBuilder data_block_;
    std::unique_ptr<BloomFilter> bloom_;
    std::string index_block_;
    std::string last_key_;
    uint64_t offset_;
    bool finished_;
    uint64_t entry_count_;
};

}  // namespace core
}  // namespace minikv
