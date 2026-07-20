#pragma once
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "core/skip_list.h"
#include "minikv/slice.h"

namespace minikv {
namespace core {

struct MemTableEntry {
    uint64_t internal_key;
    std::string value;
};

class MemTable {
public:
    explicit MemTable(size_t max_size = 4 * 1024 * 1024);

    void put(const Slice& userKey, const Slice& value, uint64_t seq, bool isDelete);
    std::optional<std::string> get(const Slice& userKey, uint64_t seq) const;
    std::vector<MemTableEntry> entries() const;
    size_t approximateMemoryUsage() const;
    bool shouldFlush() const;
    bool empty() const;

private:
    std::unique_ptr<SkipList> table_;
    size_t max_size_;
    std::atomic<uint64_t> entry_count_{0};
    mutable std::shared_mutex mutex_;
};

}  // namespace core
}  // namespace minikv
