#include <gtest/gtest.h>

#include <random>
#include <string>

#include "core/compression.h"

using minikv::core::CompressionType;
using minikv::core::compressBlock;
using minikv::core::decompressBlock;

namespace {

std::string makeRandom(size_t n) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, 255);
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; ++i) s.push_back(static_cast<char>(dist(rng)));
    return s;
}

std::string makeText(size_t n) {
    const char* kSample =
        "the quick brown fox jumps over the lazy dog 0123456789";
    std::string s;
    s.reserve(n);
    while (s.size() < n) {
        s.append(kSample);
    }
    s.resize(n);
    return s;
}

}  // namespace

TEST(CompressionTest, NoneIsIdentity) {
    std::string input = makeText(4096);
    std::string out;
    ASSERT_TRUE(compressBlock(CompressionType::kNone, input, out).ok());
    EXPECT_EQ(out, input);

    std::string back;
    ASSERT_TRUE(decompressBlock(CompressionType::kNone, out,
                                 input.size(), back).ok());
    EXPECT_EQ(back, input);
}

TEST(CompressionTest, SnappyRoundTripRandom) {
    std::string input = makeRandom(4096);
    std::string out;
    ASSERT_TRUE(compressBlock(CompressionType::kSnappy, input, out).ok());

    // Random data does not compress well; just verify size is bounded.
    EXPECT_LE(out.size(), input.size() + 64);

    std::string back;
    ASSERT_TRUE(decompressBlock(CompressionType::kSnappy, out,
                                 input.size(), back).ok());
    EXPECT_EQ(back, input);
}

TEST(CompressionTest, SnappyRoundTripText) {
    std::string input = makeText(64 * 1024);
    std::string out;
    ASSERT_TRUE(compressBlock(CompressionType::kSnappy, input, out).ok());
    // Text should compress to well under 50% of the original size.
    EXPECT_LT(out.size(), input.size() / 2);

    std::string back;
    ASSERT_TRUE(decompressBlock(CompressionType::kSnappy, out,
                                 input.size(), back).ok());
    EXPECT_EQ(back, input);
}

TEST(CompressionTest, ZstdRoundTripText) {
    std::string input = makeText(64 * 1024);
    std::string out;
    ASSERT_TRUE(compressBlock(CompressionType::kZstd, input, out).ok());
    // Zstd should do at least as well as snappy on highly repetitive text.
    EXPECT_LT(out.size(), input.size() / 2);

    std::string back;
    ASSERT_TRUE(decompressBlock(CompressionType::kZstd, out,
                                 input.size(), back).ok());
    EXPECT_EQ(back, input);
}

TEST(CompressionTest, DecompressWrongTypeFails) {
    std::string input = makeText(512);
    std::string compressed;
    ASSERT_TRUE(compressBlock(CompressionType::kSnappy, input, compressed).ok());

    std::string back;
    // Lie about the type — should fail rather than return garbage.
    auto s = decompressBlock(CompressionType::kZstd, compressed,
                              input.size(), back);
    EXPECT_FALSE(s.ok());
}

TEST(CompressionTest, DecompressWrongSizeFails) {
    std::string input = makeText(512);
    std::string compressed;
    ASSERT_TRUE(compressBlock(CompressionType::kSnappy, input, compressed).ok());

    std::string back;
    auto s = decompressBlock(CompressionType::kSnappy, compressed,
                              input.size() + 100, back);
    EXPECT_FALSE(s.ok());
}

TEST(CompressionTest, EmptyInput) {
    std::string input;
    for (auto t : {CompressionType::kNone, CompressionType::kSnappy,
                    CompressionType::kZstd}) {
        std::string out;
        ASSERT_TRUE(compressBlock(t, input, out).ok());
        std::string back;
        ASSERT_TRUE(decompressBlock(t, out, 0, back).ok());
        EXPECT_TRUE(back.empty());
    }
}