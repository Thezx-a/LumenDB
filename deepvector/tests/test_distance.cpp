#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <random>
#include "dv/index/distance.h"

using namespace dv::index;

TEST(DistanceTest, L2DistanceZero) {
    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    EXPECT_FLOAT_EQ(l2_distance(a.data(), a.data(), 3), 0.0f);
}

TEST(DistanceTest, L2DistanceKnown) {
    std::vector<float> a = {0.0f, 0.0f, 0.0f};
    std::vector<float> b = {3.0f, 4.0f, 0.0f};
    EXPECT_FLOAT_EQ(l2_distance(a.data(), b.data(), 3), 5.0f);
}

TEST(DistanceTest, L2Squared) {
    std::vector<float> a = {1.0f, 0.0f};
    std::vector<float> b = {4.0f, 0.0f};
    EXPECT_FLOAT_EQ(l2_squared(a.data(), b.data(), 2), 9.0f);
}

TEST(DistanceTest, InnerProduct) {
    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};
    float expected = -(1*4 + 2*5 + 3*6); // negated
    EXPECT_FLOAT_EQ(inner_product(a.data(), b.data(), 3), expected);
}

TEST(DistanceTest, CosineIdentical) {
    std::vector<float> a = {1.0f, 2.0f};
    EXPECT_FLOAT_EQ(cosine_distance(a.data(), a.data(), 2), 0.0f);
}

TEST(DistanceTest, CosineOrthogonal) {
    std::vector<float> a = {1.0f, 0.0f};
    std::vector<float> b = {0.0f, 1.0f};
    EXPECT_FLOAT_EQ(cosine_distance(a.data(), b.data(), 2), 1.0f);
}

TEST(DistanceTest, SIMDvsScalarConsistency) {
    const int dim = 128;
    std::vector<float> a(dim), b(dim);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (int i = 0; i < dim; ++i) {
        a[i] = dist(rng);
        b[i] = dist(rng);
    }
    float l2 = l2_distance(a.data(), b.data(), dim);
    float l2sq = l2_squared(a.data(), b.data(), dim);
    EXPECT_NEAR(l2 * l2, l2sq, 1e-4f);
}
