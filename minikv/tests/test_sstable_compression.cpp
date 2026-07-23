#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "core/compression.h"
#include "core/internal_key.h"
#include "core/sstable_builder.h"
#include "core/sstable_reader.h"
#include "minikv/slice.h"

using minikv::core::CompressionType;
using minikv::core::InternalKeyEncode;
using minikv::core::SSTableBuilder;
using minikv::core::SSTableReader;
using minikv::core::ValueType;
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
    // Zero-pad so user keys are in ascending order (SSTable requires sorted adds).
    for (int i = 0; i < 100; ++i) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "key%03d", i);
        std::string userKey(buf);
        std::string value   = "value" + std::to_string(i) +
                              std::string(64, static_cast<char>('a' + i % 26));
        std::string ikey = InternalKeyEncode(userKey, static_cast<uint64_t>(i), ValueType::kValue);
        b.add(Slice(ikey), Slice(userKey), Slice(value));
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

TEST(SSTableCompressionTest, PointLookupByUserKey) {
    std::string path = tmpDir() + "_lookup.sst";
    cleanup(path);
    buildSample(path, CompressionType::kNone);

    auto r = SSTableReader::open(path);
    ASSERT_NE(r, nullptr);

    auto v = r->get(Slice("key050"));
    ASSERT_TRUE(v.has_value());
    EXPECT_TRUE(v->find("value50") == 0);

    EXPECT_FALSE(r->get(Slice("key999")).has_value());

    cleanup(path);
}

TEST(SSTableCompressionTest, PointLookupReturnsNewestVersion) {
    std::string path = tmpDir() + "_multi.sst";
    cleanup(path);

    SSTableBuilder b(path, /*block_size=*/256, CompressionType::kNone);
    std::string userKey = "dup";
    // Internal keys for the same user key must be added in descending seq order.
    std::string ikey1 = InternalKeyEncode(userKey, 1, ValueType::kValue);
    std::string ikey2 = InternalKeyEncode(userKey, 2, ValueType::kValue);
    std::string ikey3 = InternalKeyEncode(userKey, 3, ValueType::kDeletion);
    b.add(Slice(ikey3), Slice(userKey), Slice(""));
    b.add(Slice(ikey2), Slice(userKey), Slice("new"));
    b.add(Slice(ikey1), Slice(userKey), Slice("old"));
    auto s = b.finish();
    ASSERT_TRUE(s.ok()) << s.message();

    auto r = SSTableReader::open(path);
    ASSERT_NE(r, nullptr);

    auto v = r->get(Slice("dup"));
    EXPECT_FALSE(v.has_value());

    cleanup(path);
}
