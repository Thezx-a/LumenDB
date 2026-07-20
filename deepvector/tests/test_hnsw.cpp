#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <random>
#include "dv/index/hnsw.h"
#include "dv/index/distance.h"

using namespace lumendb;
using namespace dv::index;

class HNSWTest : public ::testing::Test {
protected:
    void SetUp() override {
        dim_ = 16;
        std::mt19937 rng(42);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        for (int i = 0; i < 200; ++i) {
            std::vector<float> v(dim_);
            for (size_t d = 0; d < dim_; ++d) v[d] = dist(rng);
            // Normalize
            float norm = 0;
            for (size_t d = 0; d < dim_; ++d) norm += v[d] * v[d];
            norm = std::sqrt(norm);
            for (size_t d = 0; d < dim_; ++d) v[d] /= norm;
            vectors_.push_back(std::move(v));
        }
    }

    size_t dim_;
    std::vector<std::vector<float>> vectors_;
};

TEST_F(HNSWTest, InsertAndSearch) {
    HNSWIndex idx(16, 200);
    idx.setEfSearch(50);

    // Store vectors in a side array for distance computation
    std::vector<const float*> vec_ptrs;
    for (auto& v : vectors_) vec_ptrs.push_back(v.data());

    idx.setPairwiseDistance([&](uint64_t a, uint64_t b) {
        return l2_squared(vec_ptrs[a-1], vec_ptrs[b-1], dim_);
    });
    idx.setQueryDistance([&](uint64_t id, const float* q) {
        return l2_squared(vec_ptrs[id-1], q, dim_);
    });

    for (size_t i = 0; i < vectors_.size(); ++i) {
        idx.insert(i + 1);
    }
    EXPECT_EQ(idx.size(), 200);

    // Search with the first vector as query
    auto results = idx.search(vectors_[0].data(), 5);
    ASSERT_GE(results.size(), 1u);
    EXPECT_EQ(results[0].id, 1u); // Should find itself
    EXPECT_NEAR(results[0].distance, 0.0f, 0.001f);
}

TEST_F(HNSWTest, RecallTest) {
    HNSWIndex idx(16, 200);
    idx.setEfSearch(50);

    std::vector<const float*> vec_ptrs;
    for (auto& v : vectors_) vec_ptrs.push_back(v.data());

    idx.setPairwiseDistance([&](uint64_t a, uint64_t b) {
        return l2_squared(vec_ptrs[a-1], vec_ptrs[b-1], dim_);
    });
    idx.setQueryDistance([&](uint64_t id, const float* q) {
        return l2_squared(vec_ptrs[id-1], q, dim_);
    });

    for (size_t i = 0; i < vectors_.size(); ++i) idx.insert(i + 1);

    // Test recall on last 20 vectors
    int hits = 0;
    for (size_t i = 180; i < 200; ++i) {
        auto results = idx.search(vectors_[i].data(), 10);
        for (auto& r : results) {
            if (r.id == i + 1) { hits++; break; }
        }
    }
    // At least 50% recall@10
    EXPECT_GE(hits, 10);
}

TEST(HNSWEmpty, EmptySearch) {
    HNSWIndex idx(16, 200);
    std::vector<float> q = {1.0f, 0.0f};
    auto results = idx.search(q.data(), 5);
    EXPECT_TRUE(results.empty());
}
