#include "distance.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <chrono>
#include <random>

int main() {
    const size_t DIM = 1024;
    const size_t N = 100000;

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<float> va(DIM), vb(DIM);
    for (size_t i = 0; i < DIM; i++) {
        va[i] = dist(rng);
        vb[i] = dist(rng);
    }

    std::cout << "=== Correctness Check ===" << std::endl;
    std::cout << "L2 squared: " << l2_squared(va.data(), vb.data(), DIM) << std::endl;
    std::cout << "IP (scalar): " << ip_distance(va.data(), vb.data(), DIM) << std::endl;
    std::cout << "IP (AVX2):   " << ip_distance_avx2(va.data(), vb.data(), DIM) << std::endl;
    std::cout << "Cosine:      " << cosine_distance(va.data(), vb.data(), DIM) << std::endl;

    std::cout << "\n=== Performance (" << N << " iterations x " << DIM << " dims) ===" << std::endl;
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        volatile float sink = 0;
        for (size_t i = 0; i < N; i++)
            sink += ip_distance(va.data(), vb.data(), DIM);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "Scalar: " << us / 1000.0 << " ms" << std::endl;
    }
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        volatile float sink = 0;
        for (size_t i = 0; i < N; i++)
            sink += ip_distance_avx2(va.data(), vb.data(), DIM);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "AVX2:   " << us / 1000.0 << " ms" << std::endl;
    }

    return 0;
}
