#include "core/memtable.h"

namespace minikv {
namespace core {

MemTable::MemTable(size_t max_size)
    : table_(std::make_unique<SkipList>()), max_size_(max_size) {}

void MemTable::put(const Slice& userKey, const Slice& value, uint64_t seq, bool isDelete) {
    ValueType type = isDelete ? ValueType::kDeletion : ValueType::kValue;
    std::string ikey = InternalKeyEncode(userKey, seq, type);
    std::string val = isDelete ? "" : value.toString();
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        table_->put(ikey, val);
        entry_count_++;
    }
}

std::optional<std::string> MemTable::get(const Slice& userKey, uint64_t seq) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto entries = table_->entries();
    for (auto& [ik, v] : entries) {
        Slice ikSlice(ik);
        Slice uk = InternalKeyUserKey(ikSlice);
        int cmp = uk.compare(userKey);
        if (cmp < 0) continue;
        if (cmp > 0) break;
        if (IsDeletion(ikSlice)) return std::nullopt;
        return v;
    }
    return std::nullopt;
}

std::vector<MemTableEntry> MemTable::entries() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto raw = table_->entries();
    std::vector<MemTableEntry> result;
    result.reserve(raw.size());
    for (auto& [k, v] : raw) result.push_back({std::move(k), std::move(v)});
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