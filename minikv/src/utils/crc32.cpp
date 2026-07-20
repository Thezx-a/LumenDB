#include "utils/crc32.h"
#include <mutex>

namespace minikv {
namespace utils {

static uint32_t crc_table[256];
static std::once_flag crc_init_flag;

static void initCRC() {
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k) {
            if (c & 1) c = 0x82F63B78 ^ (c >> 1);
            else c >>= 1;
        }
        crc_table[i] = c;
    }
}

uint32_t crc32c(const char* data, int len) {
    std::call_once(crc_init_flag, initCRC);
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < len; ++i)
        crc = crc_table[(crc ^ static_cast<unsigned char>(data[i])) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

}  // namespace utils
}  // namespace minikv
