#include <gtest/gtest.h>
#include "utils/lru_cache.h"
using namespace minikv::utils;

TEST(LRUCacheTest, PutAndGet) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");
    EXPECT_EQ(cache.get(1).value(), "one");
}

TEST(LRUCacheTest, Eviction) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one"); cache.put(2, "two"); cache.put(3, "three");
    EXPECT_FALSE(cache.get(1).has_value());
}

TEST(LRUCacheTest, LRUOrder) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one"); cache.put(2, "two");
    cache.get(1); cache.put(3, "three");
    EXPECT_EQ(cache.get(1).value(), "one");
    EXPECT_FALSE(cache.get(2).has_value());
}
