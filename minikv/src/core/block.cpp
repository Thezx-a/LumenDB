#include "core/block.h"
#include <algorithm>
#include <cstring>
#include "utils/coding.h"
#include "utils/crc32.h"

namespace minikv {
namespace core {

static const int kRestartInterval = 16;

BlockBuilder::BlockBuilder(size_t block_size)
    : block_size_(block_size), finished_(false) {}

void BlockBuilder::add(const Slice& key, const Slice& value) {
    size_t shared = 0;
    if (!last_key_.empty()) {
        size_t minLen = std::min(last_key_.size(), key.size());
        while (shared < minLen && last_key_[shared] == key[shared]) ++shared;
    }
    size_t non_shared = key.size() - shared;
    std::string header;
    utils::encodeVariant32(header, static_cast<uint32_t>(shared));
    utils::encodeVariant32(header, static_cast<uint32_t>(non_shared));
    utils::encodeVariant32(header, static_cast<uint32_t>(value.size()));
    buffer_.append(header);
    buffer_.append(key.data() + shared, non_shared);
    buffer_.append(value.data(), value.size());
    last_key_ = key.toString();
}

Slice BlockBuilder::finish() {
    finished_ = true;
    return Slice(buffer_);
}

BlockReader::BlockReader(const Slice& block_data)
    : data_(block_data), num_entries_(0), restarts_offset_(0) {
    if (block_data.size() < 4) return;
    uint32_t restarts_count = utils::decodeFixed32(
        block_data.data() + block_data.size() - 4);
    restarts_offset_ = static_cast<uint32_t>(block_data.size() - 4 - restarts_count * 4);
    num_entries_ = restarts_count;
}

std::optional<std::string> BlockReader::get(const Slice& key) const {
    size_t offset = 0;
    std::string lastKey;
    while (offset < restarts_offset_) {
        uint32_t shared, nonShared, valLen;
        const char* p = data_.data() + offset;
        const char* limit = data_.data() + restarts_offset_;
        uint32_t consumed = 0;
        if (!utils::decodeVariant32(p, limit, shared, consumed)) break;
        p += consumed;
        if (!utils::decodeVariant32(p, limit, nonShared, consumed)) break;
        p += consumed;
        if (!utils::decodeVariant32(p, limit, valLen, consumed)) break;
        p += consumed;
        std::string currentKey = lastKey.substr(0, shared);
        currentKey.append(p, nonShared);
        p += nonShared;
        if (currentKey == key.toString()) {
            return std::string(p, valLen);
        }
        if (currentKey > key.toString()) return std::nullopt;
        lastKey = std::move(currentKey);
        offset = (p - data_.data()) + valLen;
    }
    return std::nullopt;
}

}  // namespace core
}  // namespace minikv
