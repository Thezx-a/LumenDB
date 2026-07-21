#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "core/internal_key.h"

using namespace minikv::core;

TEST(InternalKeyTest, EncodeDecodeRoundTrip) {
    std::string ikey = InternalKeyEncode("hello", 42, ValueType::kValue);
    EXPECT_EQ(ikey.size(), 5u + 8u);
    EXPECT_EQ(InternalKeyUserKey(ikey).toString(), "hello");
    EXPECT_EQ(InternalKeySequence(ikey), 42u);
    EXPECT_EQ(InternalKeyType(ikey), ValueType::kValue);
}

TEST(InternalKeyTest, HighSeqSortsBefore) {
    std::string older = InternalKeyEncode("k", 10, ValueType::kValue);
    std::string newer = InternalKeyEncode("k", 11, ValueType::kValue);
    // newer should sort BEFORE older (descending seq).
    EXPECT_LT(InternalKeyCompare(newer, older), 0);
    EXPECT_GT(InternalKeyCompare(older, newer), 0);
}

TEST(InternalKeyTest, UserKeyAscendingPreserved) {
    std::string a = InternalKeyEncode("a", 1000, ValueType::kValue);
    std::string b = InternalKeyEncode("b", 1, ValueType::kValue);
    EXPECT_LT(InternalKeyCompare(a, b), 0);
    EXPECT_GT(InternalKeyCompare(b, a), 0);
}

TEST(InternalKeyTest, TypeWithinSameUserKey) {
    std::string v = InternalKeyEncode("k", 5, ValueType::kValue);
    std::string d = InternalKeyEncode("k", 5, ValueType::kDeletion);
    // Lower ValueType enum wins, so kValue (1) sorts before kDeletion (2).
    EXPECT_LT(InternalKeyCompare(v, d), 0);
}

TEST(InternalKeyTest, SortMixedKeysStable) {
    std::vector<std::string> keys = {
        InternalKeyEncode("zebra", 10, ValueType::kValue),   // user_key "zebra"
        InternalKeyEncode("apple", 99, ValueType::kValue),  // user_key "apple" low
        InternalKeyEncode("apple", 5,  ValueType::kDeletion), // "apple" older delete
        InternalKeyEncode("apple", 99, ValueType::kDeletion),// same seq delete
        InternalKeyEncode("mango",  50, ValueType::kValue),
        InternalKeyEncode("mango", 50, ValueType::kDeletion),
    };
    std::vector<size_t> expected = {1, 4, 5, 2, 3, 0};
    // 1: apple@99   value   (newest)
    // 4: mango@50   value?  Wait — kValue@50 < kDel@50  so:  value mango@50 sorts before mango@50 del
    // 5: mango@50   delete
    // 2: apple@5    del (oldest apple)
    // 3: apple@99   delete (same seq)
    //   Expected order: apple v@99 → apple v? no wait apple99 v and apple99 del
    //   Actually let me re-derive:
    //     apple: (a@99 v), (a@5 d)?? wait checked: apple@5 del vs apple@99 v vs apple@99 del
    //     sort: user=apple: seq desc → 99 first, then 5
    //       at seq 99: type asc: v then d → apple@99 v first, then apple@99 d
    //       at seq 5: type asc: del here, so d → apple@5 d
    //     apple@99 v (idx 1), apple@99 d (idx 3), apple@5 d (idx 2)
    //   mango: same seq 50: v (idx 4), d (idx 5)
    //   zebra: (idx 0)
    // Top-to-bottom full order: apple@99 v, apple@99 d, apple@5 d, mango@50 v, mango@50 d, zebra@10 v
    std::vector<size_t> revised = {1, 3, 2, 4, 5, 0};
    EXPECT_EQ(expected.size(), keys.size());
    // Sort by comparator; verify our derivation is the actual output.
    std::vector<size_t> idx(keys.size());
    for (size_t i = 0; i < keys.size(); ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(),
              [&](size_t x, size_t y) {
                  return InternalKeyCompare(keys[x], keys[y]) < 0;
              });
    EXPECT_EQ(idx, revised);
}

TEST(InternalKeyTest, EmptyUserKeyOk) {
    std::string ikey = InternalKeyEncode("", 5, ValueType::kDeletion);
    EXPECT_EQ(ikey.size(), 8u);
    EXPECT_TRUE(InternalKeyUserKey(ikey).empty());
    EXPECT_EQ(InternalKeySequence(ikey), 5u);
    EXPECT_EQ(InternalKeyType(ikey), ValueType::kDeletion);
    EXPECT_TRUE(IsDeletion(ikey));
}

TEST(InternalKeyTest, LargeSequence) {
    uint64_t big = (uint64_t{1} << 55);  // 36 PB-ish.
    std::string ikey = InternalKeyEncode("k", big, ValueType::kValue);
    EXPECT_EQ(InternalKeySequence(ikey), big);
}

TEST(InternalKeyTest, CorruptKeySafeDecoding) {
    std::string too_short = "abc";  // < 8 bytes
    EXPECT_EQ(InternalKeyUserKey(too_short).size(), 0u);
    EXPECT_EQ(InternalKeySequence(too_short), 0u);
    EXPECT_EQ(InternalKeyType(too_short), ValueType::kNone);
    EXPECT_EQ(InternalKeyCompare(Slice(too_short), Slice("abc")), 0);
}

TEST(InternalKeyTest, IsDeletionHelper) {
    std::string v = InternalKeyEncode("k", 1, ValueType::kValue);
    std::string d = InternalKeyEncode("k", 1, ValueType::kDeletion);
    EXPECT_FALSE(IsDeletion(v));
    EXPECT_TRUE(IsDeletion(d));
}