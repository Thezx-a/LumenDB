#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <random>
#include "dv/quantize/pq.h"
#include "dv/quantize/scalar.h"

using namespace dv::quantize;

TEST(PQTest, TrainEncodeDecode) {
    const int dim = 16, n = 1000;
    std::mt19937 rng(42);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> data(n * dim);
    for (int i = 0; i < n * dim; ++i) data[i] = dist(rng);

    ProductQuantizer pq(dim, 4, 256);
    pq.train(data.data(), n);

    std::vector<uint8_t> codes(pq.M());
    pq.encode(data.data(), codes.data());

    std::vector<float> decoded(dim);
    pq.decode(codes.data(), decoded.data());

    float l2 = 0;
    for (int i = 0; i < dim; ++i) {
        float diff = data[i] - decoded[i];
        l2 += diff * diff;
    }
    EXPECT_LT(std::sqrt(l2), 15.0f);
}

TEST(PQTest, DistanceConsistency) {
    const int dim = 16, n = 500;
    std::mt19937 rng(43);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> data(n * dim);
    for (int i = 0; i < n * dim; ++i) data[i] = dist(rng);

    ProductQuantizer pq(dim, 4, 256);
    pq.train(data.data(), n);

    std::vector<uint8_t> codes_a(pq.M()), codes_b(pq.M());
    pq.encode(data.data(), codes_a.data());
    pq.encode(data.data() + dim, codes_b.data());

    float symDist = pq.symmetricDistance(codes_a.data(), codes_b.data());
    EXPECT_GE(symDist, 0.0f);

    std::vector<float> distTable(pq.M() * pq.K());
    pq.computeDistanceTable(data.data(), distTable.data());
    float asymDist = pq.asymmetricDistance(codes_b.data(), distTable.data());
    EXPECT_GE(asymDist, 0.0f);
}

TEST(SQTest, TrainEncodeDecode) {
    const int dim = 8, n = 500;
    std::vector<float> data(n * dim);
    for (int i = 0; i < n * dim; ++i) data[i] = static_cast<float>(i % 200) / 10.0f;

    ScalarQuantizer sq(dim);
    sq.train(data.data(), n);

    std::vector<int8_t> codes(dim);
    sq.encode(data.data(), codes.data());

    std::vector<float> decoded(dim);
    sq.decode(codes.data(), decoded.data());

    for (int i = 0; i < dim; ++i) {
        EXPECT_NEAR(data[i], decoded[i], 1.0f);
    }
}

TEST(SQTest, DistancePositive) {
    const int dim = 8, n = 200;
    std::vector<float> data(n * dim);
    for (int i = 0; i < n * dim; ++i) data[i] = static_cast<float>(i % 100);

    ScalarQuantizer sq(dim);
    sq.train(data.data(), n);

    std::vector<int8_t> codes_a(dim), codes_b(dim);
    sq.encode(data.data(), codes_a.data());
    sq.encode(data.data() + dim, codes_b.data());

    float d = sq.l2SquaredDistance(codes_a.data(), codes_b.data());
    EXPECT_GE(d, 0.0f);
}
