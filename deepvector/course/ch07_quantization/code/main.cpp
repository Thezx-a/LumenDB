#include "pq.h"
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <cmath>
#include <cassert>

// Generate random vectors
std::vector<std::vector<float>> generate_vectors(size_t N, int D, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    std::vector<std::vector<float>> vectors(N, std::vector<float>(D));
    for (auto& v : vectors) {
        for (auto& x : v) x = dist(rng);
    }
    return vectors;
}

// Compute L2 distance between two float vectors
float l2_distance(const float* a, const float* b, int D) {
    float sum = 0.0f;
    for (int i = 0; i < D; ++i) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum);
}

// Compute exact top-K via brute force
std::vector<int> brute_force_topk(const float* query, const std::vector<std::vector<float>>& db, int K) {
    size_t N = db.size();
    int D = static_cast<int>(db[0].size());
    std::vector<std::pair<float, int>> dists(N);
    for (size_t i = 0; i < N; ++i) {
        dists[i] = {l2_distance(query, db[i].data(), D), static_cast<int>(i)};
    }
    std::partial_sort(dists.begin(), dists.begin() + K, dists.end());
    std::vector<int> result(K);
    for (int i = 0; i < K; ++i) result[i] = dists[i].second;
    return result;
}

// Compute recall@K
double recall_at_k(const std::vector<int>& true_topk, const std::vector<std::pair<float, int>>& pq_topk) {
    std::unordered_set<int> pq_set;
    for (const auto& [dist, idx] : pq_topk) pq_set.insert(idx);
    int hits = 0;
    for (int idx : true_topk) {
        if (pq_set.count(idx)) hits++;
    }
    return static_cast<double>(hits) / true_topk.size();
}

// Compute compression ratio
double compression_ratio(int D, int M) {
    double original = D * sizeof(float);
    double compressed = M * sizeof(uint8_t);
    return original / compressed;
}

int main() {
    const int D = 64;
    const int M = 16;       // 16 subspaces, each 4-dimensional
    const int dsub = D / M; // 4
    const size_t N = 10000;
    const size_t N_QUERY = 200;
    const int TOP_K = 10;

    std::cout << "=== Product Quantization Demo ===" << std::endl;
    std::cout << "D=" << D << ", M=" << M << ", dsub=" << dsub << ", K=256" << std::endl;
    std::cout << "Database size: " << N << " vectors" << std::endl;
    std::cout << "Compression ratio: " << compression_ratio(D, M) << "x" << std::endl;
    std::cout << std::endl;

    // Generate data
    std::cout << "Generating vectors..." << std::endl;
    auto db = generate_vectors(N, D, 42);
    auto queries = generate_vectors(N_QUERY, D, 123);

    // Train PQ
    std::cout << "Training Product Quantizer..." << std::endl;
    ProductQuantizer pq(D, M);
    auto t0 = std::chrono::steady_clock::now();
    pq.Train(db[0].data(), N);
    auto t1 = std::chrono::steady_clock::now();
    double train_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Training time: " << train_ms << " ms" << std::endl;

    // Encode all vectors
    std::cout << "Encoding " << N << " vectors..." << std::endl;
    std::vector<std::vector<uint8_t>> all_codes(N, std::vector<uint8_t>(M));
    for (size_t i = 0; i < N; ++i) {
        pq.Encode(db[i].data(), all_codes[i].data());
    }

    // Measure storage savings
    size_t original_bytes = N * D * sizeof(float);
    size_t compressed_bytes = N * M * sizeof(uint8_t);
    std::cout << "Original storage:  " << original_bytes / 1024 << " KB" << std::endl;
    std::cout << "Compressed storage: " << compressed_bytes / 1024 << " KB" << std::endl;
    std::cout << std::endl;

    // Search and measure recall
    std::cout << "Running ADC search on " << N_QUERY << " queries (top-" << TOP_K << ")..." << std::endl;
    double total_recall = 0.0;
    double total_pq_time = 0.0;
    double total_bf_time = 0.0;

    for (size_t q = 0; q < N_QUERY; ++q) {
        // Brute force ground truth
        auto t_bf0 = std::chrono::steady_clock::now();
        auto true_topk = brute_force_topk(queries[q].data(), db, TOP_K);
        auto t_bf1 = std::chrono::steady_clock::now();
        total_bf_time += std::chrono::duration<double, std::micro>(t_bf1 - t_bf0).count();

        // PQ search
        std::vector<std::pair<float, int>> pq_results;
        auto t_pq0 = std::chrono::steady_clock::now();
        pq.ADCSearch(queries[q].data(), all_codes, TOP_K, pq_results);
        auto t_pq1 = std::chrono::steady_clock::now();
        total_pq_time += std::chrono::duration<double, std::micro>(t_pq1 - t_pq0).count();

        total_recall += recall_at_k(true_topk, pq_results);
    }

    double avg_recall = total_recall / N_QUERY;
    double avg_pq_us = total_pq_time / N_QUERY;
    double avg_bf_us = total_bf_time / N_QUERY;

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Recall@" << TOP_K << ": " << avg_recall * 100 << "%" << std::endl;
    std::cout << "Avg PQ search time:  " << avg_pq_us << " us/query" << std::endl;
    std::cout << "Avg BF search time:  " << avg_bf_us << " us/query" << std::endl;
    std::cout << "Speedup: " << avg_bf_us / avg_pq_us << "x" << std::endl;
    std::cout << "Compression: " << compression_ratio(D, M) << "x" << std::endl;

    // Reconstruction error demo
    std::cout << "\n=== Reconstruction Error Demo ===" << std::endl;
    float total_error = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        std::vector<float> reconstructed(D);
        pq.Decode(all_codes[i].data(), reconstructed.data());
        float err = l2_distance(db[i].data(), reconstructed.data(), D);
        total_error += err;
    }
    std::cout << "Avg L2 reconstruction error: " << total_error / N << std::endl;

    std::cout << "\nAll PQ demos completed successfully!" << std::endl;
    return 0;
}
