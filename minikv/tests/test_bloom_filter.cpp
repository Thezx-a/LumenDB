#include <gtest/gtest.h>
#include <random>
#include <unordered_set>
#include "core/bloom_filter.h"
using namespace minikv::core;

TEST(BloomFilterTest, BasicContains) {
    BloomFilter bf(1000, 0.01);
    bf.add(minikv::Slice("hello"));
    EXPECT_TRUE(bf.mightContain(minikv::Slice("hello")));
}

TEST(BloomFilterTest, FalsePositiveRate) {
    BloomFilter bf(100000, 0.01);
    std::unordered_set<std::string> keys;
    std::mt19937 rng(42);
    for (int i = 0; i < 100000; ++i) {
        std::string k = "key_" + std::to_string(rng());
        keys.insert(k);
        bf.add(minikv::Slice(k));
    }
    int falsePositives = 0;
    for (int i = 0; i < 100000; ++i) {
        std::string k = "missing_" + std::to_string(rng());
        if (keys.count(k)) continue;
        if (bf.mightContain(minikv::Slice(k))) ++falsePositives;
    }
    double rate = double(falsePositives) / 100000;
    std::cout << "False positive rate: " << rate << std::endl;
    EXPECT_LT(rate, 0.012);
}
