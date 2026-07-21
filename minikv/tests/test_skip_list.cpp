#include <gtest/gtest.h>
#include <random>
#include "core/skip_list.h"
using namespace minikv::core;

TEST(SkipListTest, PutAndGet) {
    SkipList sl;
    sl.put("key1", "one");
    sl.put("key2", "two");
    EXPECT_EQ(sl.get("key1").value(), "one");
    EXPECT_EQ(sl.get("key2").value(), "two");
}

TEST(SkipListTest, GetMissing) {
    SkipList sl;
    sl.put("key1", "one");
    EXPECT_FALSE(sl.get("key99").has_value());
}

TEST(SkipListTest, Del) {
    SkipList sl;
    sl.put("key1", "one");
    sl.del("key1");
    EXPECT_FALSE(sl.get("key1").has_value());
}

TEST(SkipListTest, LargeInsert) {
    SkipList sl;
    std::mt19937 rng(42);
    std::vector<std::string> keys;
    for (int i = 0; i < 10000; ++i) {
        std::string k = "key_" + std::to_string(rng() % 100000);
        sl.put(k, "val_" + k);
        keys.push_back(k);
    }
    for (const auto& k : keys) {
        auto v = sl.get(k);
        ASSERT_TRUE(v.has_value()) << "key=" << k;
    }
}

TEST(SkipListTest, EntriesOrdered) {
    SkipList sl;
    sl.put("c", "val_c");
    sl.put("a", "val_a");
    sl.put("b", "val_b");
    auto entries = sl.entries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].first, "a");
    EXPECT_EQ(entries[1].first, "b");
    EXPECT_EQ(entries[2].first, "c");
}

TEST(SkipListTest, InternalKeyOrdering) {
    std::string k1 = InternalKeyEncode("user", 10, ValueType::kValue);
    std::string k2 = InternalKeyEncode("user", 5, ValueType::kValue);
    std::string k3 = InternalKeyEncode("user", 5, ValueType::kDeletion);

    SkipList sl;
    sl.put(k3, "");
    sl.put(k1, "val1");
    sl.put(k2, "val2");

    auto entries = sl.entries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].first, k1);
    EXPECT_EQ(entries[1].first, k2);
    EXPECT_EQ(entries[2].first, k3);
}

TEST(SkipListTest, UpdateOverwrites) {
    SkipList sl;
    sl.put("key1", "old");
    sl.put("key1", "new");
    EXPECT_EQ(sl.get("key1").value(), "new");
    EXPECT_EQ(sl.entries().size(), 1u);
}
