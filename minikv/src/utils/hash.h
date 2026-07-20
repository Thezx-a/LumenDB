#pragma once
#include <cstdint>
#include <cstddef>

namespace minikv {
namespace utils {

inline uint32_t murmurHash2(const char* data, size_t len, uint32_t seed) {
    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h = seed ^ static_cast<uint32_t>(len);
    while (len >= 4) {
        uint32_t k = static_cast<uint32_t>(static_cast<unsigned char>(data[0])) |
                     (static_cast<uint32_t>(static_cast<unsigned char>(data[1])) << 8) |
                     (static_cast<uint32_t>(static_cast<unsigned char>(data[2])) << 16) |
                     (static_cast<uint32_t>(static_cast<unsigned char>(data[3])) << 24);
        k *= m; k ^= k >> r; k *= m;
        h *= m; h ^= k;
        data += 4; len -= 4;
    }
    switch (len) {
        case 3: h ^= static_cast<unsigned char>(data[2]) << 16; [[fallthrough]];
        case 2: h ^= static_cast<unsigned char>(data[1]) << 8; [[fallthrough]];
        case 1: h ^= static_cast<unsigned char>(data[0]); h *= m;
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

}  // namespace utils
}  // namespace minikv
