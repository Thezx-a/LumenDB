#pragma once
#include <cstdint>
#include <cstddef>

namespace minikv {
namespace utils {

uint32_t crc32c(const char* data, int len);

inline uint32_t crc32cExtend(uint32_t crc, const char* buf, int len) {
    return crc32c(buf, len) ^ crc;
}

}  // namespace utils
}  // namespace minikv
