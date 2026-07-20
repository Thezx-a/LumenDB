#include <gtest/gtest.h>
#include "utils/coding.h"
using namespace minikv::utils;

TEST(CodingTest, Fixed32RoundTrip) {
    char buf[4];
    encodeFixed32(buf, 0x12345678);
    EXPECT_EQ(decodeFixed32(buf), 0x12345678u);
}

TEST(CodingTest, Fixed64RoundTrip) {
    char buf[8];
    encodeFixed64(buf, 0xDEADBEEFCAFEULL);
    EXPECT_EQ(decodeFixed64(buf), 0xDEADBEEFCAFEULL);
}

TEST(CodingTest, Variant32RoundTrip) {
    for (uint32_t val : {0u, 1u, 127u, 128u, 255u, 256u, 16383u, 16384u, 1000000u}) {
        std::string s;
        encodeVariant32(s, val);
        const char* p = s.data();
        const char* limit = s.data() + s.size();
        uint32_t result, consumed;
        ASSERT_TRUE(decodeVariant32(p, limit, result, consumed));
        EXPECT_EQ(result, val);
    }
}
