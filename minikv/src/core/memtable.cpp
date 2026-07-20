#include "core/memtable.h"

namespace minikv {
namespace core {

MemTable::MemTable(size_t max_size)
    : table_(std::make_unique<SkipList>()), max_size_(max_size) {}

void MemTable::put(const Slice& userKey, const Slice& value, uint64_t seq, bool isDelete) {
    uint8_t type = isDelete ? 2 : 1;
    uint64_t internalKey = packInternalKey(userKey, seq, type);
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        table_->put(internalKey, isDelete ? "" : value.toString());
        entry_count_++;
    }
}

std::optional<std::string> MemTable::get(const Slice& userKey, uint64_t seq) const {
    uint64_t userKeyPart = packInternalKey(userKey, 0, 0);
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto entries = table_->entries();
        for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
            if ((it->first & ~0xFFFFULL) != userKeyPart) continue;
            uint8_t type = static_cast<uint8_t>(it->first & 0xF);
            if (type == 2) return std::nullopt;
            if (type == 1) return it->second;
        }
    }
    return std::nullopt;
}

std::vector<MemTableEntry> MemTable::entries() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto raw = table_->entries();
    std::vector<MemTableEntry> result;
    result.reserve(raw.size());
    for (auto& [k, v] : raw) result.push_back({k, std::move(v)});
    return result;
}

size_t MemTable::approximateMemoryUsage() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return table_->approximateMemoryUsage();
}

bool MemTable::shouldFlush() const {
    return approximateMemoryUsage() >= max_size_;
}

bool MemTable::empty() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return table_->empty();
}

}  // namespace core
}  // namespace minikv
