#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "core/sstable_reader.h"
#include "minikv/iterator.h"
#include "minikv/status.h"

namespace minikv {
namespace core {

// Snapshot iterator over one SSTable (keys sorted). Keeps the reader alive.
class SSTableIterator : public Iterator {
public:
    explicit SSTableIterator(std::shared_ptr<SSTableReader> reader);

    bool valid() const override;
    void seekToFirst() override;
    void seek(const Slice& target) override;
    void next() override;
    Slice key() const override;
    Slice value() const override;
    Status status() const override;

private:
    void loadEntries();

    std::shared_ptr<SSTableReader> reader_;
    std::vector<std::pair<std::string, std::string>> entries_;
    size_t index_ = 0;
    Status status_;
};

}  // namespace core
}  // namespace minikv
