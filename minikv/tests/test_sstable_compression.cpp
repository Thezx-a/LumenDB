#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "core/compression.h"
#include "core/sstable_builder.h"
#include "core/sstable_reader.h"
#include "minikv/slice.h"

using minikv::core::CompressionType;
using minikv::core::SSTableBuilder;
using minikv::core::SSTableReader;
using minikv::Slice;
using minikv::Status;

namespace {

std::string tmpDir() {
    const char* t = std::getenv("TMPDIR");
    if (!t || *t == '\0') t = "/tmp";
    return std::string(t) + "/titankv_sst_test_" +
           std::to_string(::getpid()) + "_" +
           std::to_string(reinterpret_cast<uintptr_t>(&t));
}

void cleanup(const std::string& path) {
    ::unlink(path.c_str());
    ::unlink((path + ".bloom").c_str());
}

void buildSample(const std::string& path, CompressionType type) {
    SSTableBuilder b(path, /*block_size=*/256, type);
    for (int i = 0; i < 100; ++i) {
        std::string userKey = "key" + std::to_string(i);
        std::string value   = "value" + std::to_string(i) +
                              std::string(64, static_cast<char>('a' + i % 26));
        // Reuse the existing convention: 8-byte internal key.
        char keyBuf[8] = {0};
        for (int j = 0; j < 8; ++j)
            keyBuf[j] = static_cast<char>((i >> (j * 8)) & 0xFF);
        b.add(static_cast<uint64_t>(i), Slice(keyBuf, 8), Slice(value));
    }
    auto s = b.finish();
    ASSERT_TRUE(s.ok()) << s.message();
}

}  // namespace

TEST(SSTableCompressionTest, SnappyRoundTrip) {
    std::string path = tmpDir() + "_snappy.sst";
    cleanup(path);
    buildSample(path, CompressionType::kSnappy);

    auto r = SSTableReader::open(path);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->formatVersion(), 1);

    int count = 0;
    Status s = r->scan(Slice(), Slice(),
                       [&count](const Slice& /*k*/, const Slice& v) {
                           EXPECT_TRUE(v.toString().find("value") == 0);
                           ++count;
                       });
    EXPECT_TRUE(s.ok()) << s.message();
    EXPECT_EQ(count, 100);

    cleanup(path);
}

TEST(SSTableCompressionTest, ZstdRoundTrip) {
    std::string path = tmpDir() + "_zstd.sst";
    cleanup(path);
    buildSample(path, CompressionType::kZstd);

    auto r = SSTableReader::open(path);
    ASSERT_NE(r, nullptr);

    int count = 0;
    Status s = r->scan(Slice(), Slice(),
                       [&count](const Slice&, const Slice& v) {
                           EXPECT_EQ(v.toString().substr(0, 5), "value");
                           ++count;
                       });
    EXPECT_TRUE(s.ok()) << s.message();
    EXPECT_EQ(count, 100);

    cleanup(path);
}

TEST(SSTableCompressionTest, NoneRoundTrip) {
    std::string path = tmpDir() + "_none.sst";
    cleanup(path);
    buildSample(path, CompressionType::kNone);

    auto r = SSTableReader::open(path);
    ASSERT_NE(r, nullptr);

    int count = 0;
    Status s = r->scan(Slice(), Slice(),
                       [&count](const Slice&, const Slice&) { ++count; });
    EXPECT_TRUE(s.ok()) << s.message();
    EXPECT_EQ(count, 100);

    cleanup(path);
}

TEST(SSTableCompressionTest, CompressedFileSmallerThanRaw) {
    std::string pathRaw = tmpDir() + "_cmp_raw.sst";
    std::string pathSn  = tmpDir() + "_cmp_snappy.sst";
    cleanup(pathRaw);
    cleanup(pathSn);

    buildSample(pathRaw, CompressionType::kNone);
    buildSample(pathSn,  CompressionType::kSnappy);

    struct stat stRaw, stSn;
    ::stat(pathRaw.c_str(), &stRaw);
    ::stat(pathSn.c_str(),  &stSn);
    EXPECT_LT(stSn.st_size, stRaw.st_size);

    cleanup(pathRaw);
    cleanup(pathSn);
}