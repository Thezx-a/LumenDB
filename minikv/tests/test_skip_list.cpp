#include <gtest/gtest.h>

#include <random>
#include <string>
#include <vector>

#include "core/skip_list.h"

using namespace minikv::core;

namespace {

std::string ik(const std::string& user, uint64_t seq = 1,
               ValueType type = ValueType::kValue) {
    return InternalKeyEncode(user, seq, type);
}

}  // namespace

TEST(SkipListTest, PutAndGet) {
    SkipList sl;
    sl.put(ik("key1"), "one");
    sl.put(ik("key2"), "two");
    EXPECT_EQ(sl.get(ik("key1")).value(), "one");
    EXPECT_EQ(sl.get(ik("key2")).value(), "two");
}

TEST(SkipListTest, GetMissing) {
    SkipList sl;
    sl.put(ik("key1"), "one");
    EXPECT_FALSE(sl.get(ik("key99")).has_value());
}

TEST(SkipListTest, Del) {
    SkipList sl;
    sl.put(ik("key1"), "one");
    sl.del(ik("key1"));
    EXPECT_FALSE(sl.get(ik("key1")).has_value());
}

TEST(SkipListTest, LargeInsert) {
    SkipList sl;
    std::mt19937 rng(42);
    std::vector<std::string> keys;
    for (int i = 0; i < 10000; ++i) {
        std::string k = "key_" + std::to_string(rng() % 100000);
        sl.put(ik(k), "val_" + k);
        keys.push_back(k);
    }
    for (const auto& k : keys) {
        auto v = sl.get(ik(k));
        ASSERT_TRUE(v.has_value()) << "key=" << k;
    }
}

TEST(SkipListTest, EntriesOrdered) {
    SkipList sl;
    sl.put(ik("c"), "val_c");
    sl.put(ik("a"), "val_a");
    sl.put(ik("b"), "val_b");
    auto entries = sl.entries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(InternalKeyUserKey(entries[0].first).toString(), "a");
    EXPECT_EQ(InternalKeyUserKey(entries[1].first).toString(), "b");
    EXPECT_EQ(InternalKeyUserKey(entries[2].first).toString(), "c");
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
    sl.put(ik("key1"), "old");
    sl.put(ik("key1"), "new");
    EXPECT_EQ(sl.get(ik("key1")).value(), "new");
    EXPECT_EQ(sl.entries().size(), 1u);
}
