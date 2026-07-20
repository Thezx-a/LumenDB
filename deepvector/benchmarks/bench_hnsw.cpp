#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <cstdio>
#include "lumendb/collection.h"

using namespace lumendb;
using Clock = std::chrono::high_resolution_clock;

static std::vector<float> generateVector(size_t dim, std::mt19937& rng) {
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> v(dim);
    float norm = 0;
    for (size_t i = 0; i < dim; ++i) { v[i] = dist(rng); norm += v[i] * v[i]; }
    norm = std::sqrt(norm);
    for (size_t i = 0; i < dim; ++i) v[i] /= norm;
    return v;
}

int main() {
    ::system("rm -rf /tmp/lumendb_bench");

    const size_t dim = 128;
    const size_t N = 50000;
    const size_t query_count = 1000;
    const size_t k = 10;

    CollectionConfig config;
    config.dim = dim;
    config.metric = DistanceMetric::L2;
    config.hnsw_m = 16;
    config.hnsw_ef_construction = 200;
    config.hnsw_ef_search = 50;

    std::cout << "=== DeepVector Benchmark ===" << std::endl;
    std::cout << "Dim: " << dim << ", N: " << N << ", M: " << config.hnsw_m
              << ", ef_construction: " << config.hnsw_ef_construction << std::endl << std::endl;

    Collection coll(config, "/tmp/lumendb_bench");
    std::mt19937 rng(42);

    // Benchmark INSERT
    std::cout << "--- INSERT ---" << std::endl;
    auto t0 = Clock::now();
    std::vector<std::vector<float>> queries;
    for (size_t i = 0; i < N; ++i) {
        auto v = generateVector(dim, rng);
        if (i < query_count) queries.push_back(v);
        coll.add(v.data());
        if ((i + 1) % 10000 == 0) {
            auto elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
            std::cout << "  " << (i + 1) << " inserted in " << std::fixed << std::setprecision(2)
                      << elapsed << "s (" << static_cast<int>((i + 1) / elapsed) << " vec/s)" << std::endl;
        }
    }
    auto t1 = Clock::now();
    double insert_time = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Total: " << N << " vectors in " << std::fixed << std::setprecision(2) << insert_time
              << "s (" << static_cast<int>(N / insert_time) << " vec/s)" << std::endl << std::endl;

    // Benchmark SEARCH (batch)
    std::cout << "--- SEARCH (k=" << k << ", " << query_count << " queries) ---" << std::endl;
    std::vector<double> latencies;
    latencies.reserve(query_count);
    t0 = Clock::now();
    for (size_t i = 0; i < query_count; ++i) {
        auto q0 = Clock::now();
        auto results = coll.search(queries[i].data(), k);
        auto q1 = Clock::now();
        latencies.push_back(std::chrono::duration<double, std::micro>(q1 - q0).count());
    }
    t1 = Clock::now();
    double search_time = std::chrono::duration<double>(t1 - t0).count();
    double qps = query_count / search_time;

    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[latencies.size() / 2];
    double p95 = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];

    std::cout << "QPS: " << std::fixed << std::setprecision(0) << qps << std::endl;
    std::cout << "P50: " << std::fixed << std::setprecision(0) << p50 << " us" << std::endl;
    std::cout << "P95: " << std::fixed << std::setprecision(0) << p95 << " us" << std::endl;
    std::cout << "P99: " << std::fixed << std::setprecision(0) << p99 << " us" << std::endl;
    std::cout << "Avg: " << std::fixed << std::setprecision(0) << (search_time * 1e6 / query_count) << " us" << std::endl;
    std::cout << "Memory (index+vectors): ~" << (N * dim * 4 / 1024 / 1024) << " MB" << std::endl;

    ::system("rm -rf /tmp/lumendb_bench");
    return 0;
}
