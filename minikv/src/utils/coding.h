#pragma once
#include <cstdint>
#include <cstring>
#include <string>

namespace minikv {
namespace utils {

inline void encodeFixed32(char* buf, uint32_t val) {
    buf[0] = static_cast<char>(val & 0xFF);
    buf[1] = static_cast<char>((val >> 8) & 0xFF);
    buf[2] = static_cast<char>((val >> 16) & 0xFF);
    buf[3] = static_cast<char>((val >> 24) & 0xFF);
}

inline void encodeFixed64(char* buf, uint64_t val) {
    for (int i = 0; i < 8; ++i)
        buf[i] = static_cast<char>((val >> (i * 8)) & 0xFF);
}

inline uint32_t decodeFixed32(const char* buf) {
    return static_cast<uint32_t>(static_cast<unsigned char>(buf[0])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(buf[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(buf[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(buf[3])) << 24);
}

inline uint64_t decodeFixed64(const char* buf) {
    uint64_t result = 0;
    for (int i = 0; i < 8; ++i)
        result |= static_cast<uint64_t>(static_cast<unsigned char>(buf[i])) << (i * 8);
    return result;
}

inline void encodeVariant32(std::string& dst, uint32_t val) {
    while (val >= 0x80) {
        dst.push_back(static_cast<char>(val | 0x80));
        val >>= 7;
    }
    dst.push_back(static_cast<char>(val));
}

inline bool decodeVariant32(const char*& p, const char* limit, uint32_t& value, uint32_t& consumed) {
    value = 0;
    consumed = 0;
    uint32_t shift = 0;
    while (p < limit) {
        unsigned char c = static_cast<unsigned char>(*p++);
        ++consumed;
        if (shift >= 32) return false;
        value |= static_cast<uint32_t>(c & 0x7F) << shift;
        if ((c & 0x80) == 0) return true;
        shift += 7;
    }
    return false;
}

}  // namespace utils
}  // namespace minikv
