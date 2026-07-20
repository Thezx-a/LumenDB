#include "core/memtable_iterator.h"
#include <algorithm>
#include <cstring>

namespace minikv {
namespace core {

MemTableIterator::MemTableIterator(std::vector<MemTableEntry> entries)
    : entries_(std::move(entries)), status_(Status::Ok()) {
    std::sort(entries_.begin(), entries_.end(),
              [](const MemTableEntry& a, const MemTableEntry& b) {
                  return a.internal_key < b.internal_key;
              });
}

bool MemTableIterator::valid() const {
    return status_.ok() && index_ < entries_.size();
}

void MemTableIterator::seekToFirst() {
    index_ = 0;
    if (valid()) encodeKey(index_);
}

void MemTableIterator::seek(const Slice& target) {
    uint64_t target_key = 0;
    if (target.size() >= 8) {
        std::memcpy(&target_key, target.data(), 8);
    } else if (target.size() > 0) {
        // Treat short targets as user-key hashes packed into high bits — best-effort
        for (size_t i = 0; i < target.size() && i < 8; ++i) {
            target_key |= (static_cast<uint64_t>(
                               static_cast<unsigned char>(target.data()[i]))
                           << (i * 8));
        }
    }
    index_ = 0;
    while (index_ < entries_.size() && entries_[index_].internal_key < target_key) {
        ++index_;
    }
    if (valid()) encodeKey(index_);
}

void MemTableIterator::next() {
    if (!valid()) return;
    ++index_;
    if (valid()) encodeKey(index_);
}

Slice MemTableIterator::key() const {
    return Slice(key_buf_);
}

Slice MemTableIterator::value() const {
    if (!valid()) return Slice();
    return Slice(entries_[index_].value);
}

Status MemTableIterator::status() const {
    return status_;
}

void MemTableIterator::encodeKey(size_t idx) {
    key_buf_.assign(8, '\0');
    uint64_t ik = entries_[idx].internal_key;
    for (int i = 0; i < 8; ++i) {
        key_buf_[static_cast<size_t>(i)] =
            static_cast<char>((ik >> (i * 8)) & 0xFF);
    }
}

}  // namespace core
}  // namespace minikv
