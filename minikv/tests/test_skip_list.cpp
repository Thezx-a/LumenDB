#include <gtest/gtest.h>
#include <random>
#include "core/skip_list.h"
using namespace minikv::core;

TEST(SkipListTest, PutAndGet) {
    SkipList sl;
    sl.put(1, "one");
    sl.put(2, "two");
    EXPECT_EQ(sl.get(1).value(), "one");
    EXPECT_EQ(sl.get(2).value(), "two");
}

TEST(SkipListTest, GetMissing) {
    SkipList sl;
    sl.put(1, "one");
    EXPECT_FALSE(sl.get(42).has_value());
}

TEST(SkipListTest, Del) {
    SkipList sl;
    sl.put(1, "one");
    sl.del(1);
    EXPECT_FALSE(sl.get(1).has_value());
}

TEST(SkipListTest, LargeInsert) {
    SkipList sl;
    std::mt19937 rng(42);
    std::vector<uint64_t> keys;
    for (int i = 0; i < 10000; ++i) {
        uint64_t k = rng() % 100000;
        sl.put(k, "val_" + std::to_string(k));
        keys.push_back(k);
    }
    for (uint64_t k : keys) {
        auto v = sl.get(k);
        ASSERT_TRUE(v.has_value()) << "key=" << k;
    }
}

TEST(SkipListTest, EntriesOrdered) {
    SkipList sl;
    sl.put(3, "c"); sl.put(1, "a"); sl.put(2, "b");
    auto entries = sl.entries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].first, 1u);
    EXPECT_EQ(entries[1].first, 2u);
    EXPECT_EQ(entries[2].first, 3u);
}
