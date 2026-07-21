#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "core/memtable.h"
#include "minikv/iterator.h"
#include "minikv/status.h"

namespace minikv {
namespace core {

class MemTableIterator : public Iterator {
public:
    explicit MemTableIterator(std::vector<MemTableEntry> entries);

    bool valid() const override;
    void seekToFirst() override;
    void seek(const Slice& target) override;
    void next() override;
    Slice key() const override;
    Slice value() const override;
    Status status() const override;

private:
    std::vector<MemTableEntry> entries_;
    size_t index_ = 0;
    mutable std::string key_buf_;
    Status status_;
};

}  // namespace core
}  // namespace minikv