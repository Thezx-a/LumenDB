#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "minikv/slice.h"

namespace minikv {

enum class BatchOpType : uint8_t {
    kPut = 1,
    kDelete = 2,
};

struct BatchOp {
    BatchOpType type;
    std::string key;
    std::string value;
};

class WriteBatch {
public:
    void put(const Slice& key, const Slice& value) {
        ops_.push_back({BatchOpType::kPut, key.toString(), value.toString()});
    }
    void del(const Slice& key) {
        ops_.push_back({BatchOpType::kDelete, key.toString(), ""});
    }
    void clear() { ops_.clear(); }
    size_t count() const { return ops_.size(); }
    const std::vector<BatchOp>& ops() const { return ops_; }

private:
    std::vector<BatchOp> ops_;
};

}  // namespace minikv
