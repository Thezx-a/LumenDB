#include <gtest/gtest.h>
#include "core/memtable.h"
using namespace minikv::core;

TEST(MemTableTest, PutAndGet) {
    MemTable mt(4 * 1024 * 1024);
    mt.put(minikv::Slice("key1"), minikv::Slice("value1"), 1, false);
    auto result = mt.get(minikv::Slice("key1"), 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value1");
}

TEST(MemTableTest, Delete) {
    MemTable mt(4 * 1024 * 1024);
    mt.put(minikv::Slice("key1"), minikv::Slice("value1"), 1, false);
    mt.put(minikv::Slice("key1"), minikv::Slice(""), 2, true);
    auto result = mt.get(minikv::Slice("key1"), 2);
    EXPECT_FALSE(result.has_value());
}

TEST(MemTableTest, ShouldFlush) {
    MemTable mt(100);
    for (int i = 0; i < 100; ++i)
        mt.put(minikv::Slice("k" + std::to_string(i)), minikv::Slice(std::string(10, 'x')), i+1, false);
    EXPECT_TRUE(mt.shouldFlush());
}
