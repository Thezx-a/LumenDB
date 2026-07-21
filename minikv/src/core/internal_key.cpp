#include "core/internal_key.h"

#include <cstring>
#include <utility>

#include "utils/coding.h"

namespace minikv {
namespace core {

namespace {

// Compose the trailer uint64 from seq + type.
uint64_t packTrailer(uint64_t seq, ValueType type) {
    return (seq << kTypeBits) | static_cast<uint64_t>(type);
}

void writeTrailerLE(char* dst, uint64_t packed) {
    utils::encodeFixed64(dst, packed);
}

}  // namespace

std::string InternalKeyEncode(const Slice& user_key,
                              uint64_t     seq,
                              ValueType    type) {
    std::string out;
    out.resize(user_key.size() + kTrailerBytes);
    if (!user_key.empty()) {
        std::memcpy(&out[0], user_key.data(), user_key.size());
    }
    uint64_t packed = packTrailer(seq, type);
    char trailer[kTrailerBytes];
    writeTrailerLE(trailer, packed);
    std::memcpy(&out[user_key.size()], trailer, kTrailerBytes);
    return out;
}

Slice InternalKeyUserKey(const Slice& internal_key) {
    if (internal_key.size() < kTrailerBytes) return Slice();
    return Slice(internal_key.data(), internal_key.size() - kTrailerBytes);
}

uint64_t InternalKeySequence(const Slice& internal_key) {
    if (internal_key.size() < kTrailerBytes) return 0;
    const char* p = internal_key.data() + (internal_key.size() - kTrailerBytes);
    uint64_t packed = utils::decodeFixed64(p);
    return packed >> kTypeBits;
}

ValueType InternalKeyType(const Slice& internal_key) {
    if (internal_key.size() < kTrailerBytes) return ValueType::kNone;
    const char* p = internal_key.data() + (internal_key.size() - kTrailerBytes);
    uint64_t packed = utils::decodeFixed64(p);
    return static_cast<ValueType>(packed & ((1u << kTypeBits) - 1u));
}

int InternalKeyCompare(const Slice& a, const Slice& b) {
    Slice ua = InternalKeyUserKey(a);
    Slice ub = InternalKeyUserKey(b);
    int r = ua.compare(ub);
    if (r != 0) return r;  // user_key ascending
    if (a.empty() || b.empty()) return r;
    const char* pa = a.data() + a.size() - kTrailerBytes;
    const char* pb = b.data() + b.size() - kTrailerBytes;
    uint64_t ta = utils::decodeFixed64(pa);
    uint64_t tb = utils::decodeFixed64(pb);
    uint64_t sa = ta >> kTypeBits;
    uint64_t sb = tb >> kTypeBits;
    if (sa != sb) {
        // Descending: larger seq sorts first → result negated.
        return sa > sb ? -1 : +1;
    }
    uint8_t tya = static_cast<uint8_t>(ta & ((1u << kTypeBits) - 1u));
    uint8_t tyb = static_cast<uint8_t>(tb & ((1u << kTypeBits) - 1u));
    if (tya != tyb) return tya < tyb ? -1 : +1;
    return 0;
}

}  // namespace core
}  // namespace minikv