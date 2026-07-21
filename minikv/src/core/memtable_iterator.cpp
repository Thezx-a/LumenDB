#include "core/memtable_iterator.h"
#include "core/internal_key.h"
#include <algorithm>

namespace minikv {
namespace core {

MemTableIterator::MemTableIterator(std::vector<MemTableEntry> entries)
    : entries_(std::move(entries)), status_(Status::Ok()) {
    std::sort(entries_.begin(), entries_.end(),
              [](const MemTableEntry& a, const MemTableEntry& b) {
                  return InternalKeyCompare(Slice(a.internal_key), Slice(b.internal_key)) < 0;
              });
}

bool MemTableIterator::valid() const {
    return status_.ok() && index_ < entries_.size();
}

void MemTableIterator::seekToFirst() {
    index_ = 0;
}

void MemTableIterator::seek(const Slice& target) {
    index_ = 0;
    while (index_ < entries_.size() &&
           InternalKeyCompare(Slice(entries_[index_.internal_key]), target) < 0) {
        ++index_;
    }
}

void MemTableIterator::next() {
    if (!valid()) return;
    ++index_;
}

Slice MemTableIterator::key() const {
    if (!valid()) return Slice();
    return Slice(entries_[index_].internal_key);
}

Slice MemTableIterator::value() const {
    if (!valid()) return Slice();
    return Slice(entries_[index_].value);
}

Status MemTableIterator::status() const {
    return status_;
}

}  // namespace core
}  // namespace minikv