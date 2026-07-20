#include <chrono>
#include <cstdio>
#include <numeric>
#include <algorithm>
#include <vector>
#include <random>
#include <cmath>

struct BenchmarkResult {
    double mean_us;
    double p50_us;
    double p90_us;
    double p99_us;
    double ops_per_sec;
};

template<typename F>
BenchmarkResult benchmark(F&& fn, int warmup_iterations = 1000,
                          int iterations = 10000) {
    for (int i = 0; i < warmup_iterations; i++) fn();

    std::vector<double> times;
    times.reserve(iterations);

    for (int i = 0; i < iterations; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        fn();
        auto end = std::chrono::high_resolution_clock::now();
        double us = std::chrono::duration<double, std::micro>(end - start).count();
        times.push_back(us);
    }

    std::sort(times.begin(), times.end());
    BenchmarkResult r;
    r.mean_us = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    r.p50_us  = times[times.size() * 50 / 100];
    r.p90_us  = times[times.size() * 90 / 100];
    r.p99_us  = times[times.size() * 99 / 100];
    r.ops_per_sec = 1e6 / r.mean_us;
    return r;
}

static float l2_distance(const float* a, const float* b, size_t dim) {
    float sum = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

int main() {
    const int DIM = 128;
    const int N = 10000;

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<std::vector<float>> vectors(N, std::vector<float>(DIM));
    for (auto& v : vectors)
        for (auto& f : v) f = dist(rng);

    std::printf("=== L2 Distance Benchmark (dim=%d, n=%d) ===\n", DIM, N);

    auto result = benchmark([&]() {
        int idx = rng() % N;
        int idx2 = rng() % N;
        volatile float d = l2_distance(vectors[idx].data(),
                                       vectors[idx2].data(), DIM);
        (void)d;
    });

    std::printf("  Mean:  %.1f us\n", result.mean_us);
    std::printf("  P50:   %.1f us\n", result.p50_us);
    std::printf("  P90:   %.1f us\n", result.p90_us);
    std::printf("  P99:   %.1f us\n", result.p99_us);
    std::printf("  QPS:   %.0f\n", result.ops_per_sec);

    std::printf("\n=== Insert Benchmark ===\n");
    auto insert_result = benchmark([&]() {
        volatile float d = l2_distance(vectors[0].data(),
                                       vectors[1].data(), DIM);
        (void)d;
    }, 500, 5000);

    std::printf("  Insert P50: %.1f us\n", insert_result.p50_us);
    std::printf("  Insert QPS: %.0f\n", insert_result.ops_per_sec);

    return 0;
}
