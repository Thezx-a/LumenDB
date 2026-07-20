#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <random>
#include <cstdio>
#include "dv/collection.h"

using namespace lumendb;

TEST(CollectionTest, CreateAddSearch) {
    ::system("rm -rf /tmp/lumendb_test");
    CollectionConfig config;
    config.dim = 8;
    config.metric = DistanceMetric::L2;
    config.hnsw_m = 16;
    config.hnsw_ef_construction = 100;
    config.hnsw_ef_search = 30;

    Collection coll(config, "/tmp/lumendb_test");

    std::mt19937 rng(123);
    std::normal_distribution<float> dist(0.0f, 1.0f);

    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < 100; ++i) {
        std::vector<float> v(config.dim);
        for (size_t d = 0; d < config.dim; ++d) v[d] = dist(rng);
        vectors.push_back(v);
        coll.add(v.data());
    }
    EXPECT_EQ(coll.size(), 100u);
    EXPECT_EQ(coll.dim(), 8u);

    auto results = coll.search(vectors[0].data(), 5);
    ASSERT_GE(results.size(), 1u);

    ::system("rm -rf /tmp/lumendb_test");
}

TEST(CollectionTest, GetVector) {
    ::system("rm -rf /tmp/lumendb_test2");
    CollectionConfig config;
    config.dim = 4;
    Collection coll(config, "/tmp/lumendb_test2");

    std::vector<float> v = {1.0f, 2.0f, 3.0f, 4.0f};
    uint64_t id = coll.add(v.data());

    const float* got = coll.getVector(id);
    ASSERT_NE(got, nullptr);
    for (int i = 0; i < 4; ++i) EXPECT_FLOAT_EQ(got[i], v[i]);

    ::system("rm -rf /tmp/lumendb_test2");
}
