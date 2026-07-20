#include "core/sstable_iterator.h"
#include "core/block.h"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include "utils/coding.h"

namespace minikv {
namespace core {

SSTableIterator::SSTableIterator(std::shared_ptr<SSTableReader> reader)
    : reader_(std::move(reader)), status_(Status::Ok()) {
    if (!reader_) {
        status_ = Status::IOError("null sstable reader");
        return;
    }
    loadEntries();
}

void SSTableIterator::loadEntries() {
    // Materialize all entries via scan — correct LevelDB-style full table view.
    auto st = reader_->scan(
        Slice(), Slice(),
        [this](const Slice& k, const Slice& v) {
            entries_.emplace_back(k.toString(), v.toString());
        });
    if (!st.ok()) {
        status_ = st;
        return;
    }
    std::sort(entries_.begin(), entries_.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
}

bool SSTableIterator::valid() const {
    return status_.ok() && index_ < entries_.size();
}

void SSTableIterator::seekToFirst() { index_ = 0; }

void SSTableIterator::seek(const Slice& target) {
    std::string t = target.toString();
    index_ = 0;
    while (index_ < entries_.size() && entries_[index_].first < t) ++index_;
}

void SSTableIterator::next() {
    if (valid()) ++index_;
}

Slice SSTableIterator::key() const {
    if (!valid()) return Slice();
    return Slice(entries_[index_].first);
}

Slice SSTableIterator::value() const {
    if (!valid()) return Slice();
    return Slice(entries_[index_].second);
}

Status SSTableIterator::status() const { return status_; }

}  // namespace core
}  // namespace minikv
