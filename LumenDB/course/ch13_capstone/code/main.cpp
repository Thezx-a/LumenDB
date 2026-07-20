#include "vector_db.h"

#include <cstdio>
#include <random>
#include <unordered_set>
#include <chrono>
#include <algorithm>

int main() {
    const int D = 128;
    const int N = 1000;
    const int K = 10;

    std::printf("=== Mini Vector DB Demo ===\n\n");

    VectorStorage storage({.data_path = "demo.db", .dimension = D});
    HNSWIndex index({.dimension = D}, &storage);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<std::vector<float>> vecs(N, std::vector<float>(D));
    for (auto& v : vecs)
        for (auto& f : v) f = dist(rng);

    std::printf("Inserting %d vectors (dim=%d)...\n", N, D);
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; i++) {
        storage.append(vecs[i].data());
        index.insert(i + 1, vecs[i].data());
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double insert_sec = std::chrono::duration<double>(t1 - t0).count();
    std::printf("  Insert: %.2f s (%.0f vec/s)\n\n", insert_sec, N / insert_sec);

    int correct = 0;
    for (int i = 0; i < N; i++) {
        auto r = brute_force_search(storage, vecs[i].data(), 1);
        if (r[0].id == i + 1 && r[0].distance < 0.001f) correct++;
    }
    std::printf("Self-search accuracy: %.1f%%\n\n", 100.0 * correct / N);

    const int Q = 100;
    float total_recall = 0;
    std::vector<double> latencies;
    latencies.reserve(Q);

    for (int i = 0; i < Q; i++) {
        int qi = i * (N / Q);
        auto t_start = std::chrono::high_resolution_clock::now();
        auto approx = index.search(vecs[qi].data(), K);
        auto t_end = std::chrono::high_resolution_clock::now();
        double us = std::chrono::duration<double, std::micro>(t_end - t_start).count();
        latencies.push_back(us);

        auto ground = brute_force_search(storage, vecs[qi].data(), K);
        std::unordered_set<int64_t> truth;
        for (auto& r : ground) truth.insert(r.id);

        int match = 0;
        for (auto& r : approx) if (truth.count(r.id)) match++;
        total_recall += (float)match / K;
    }

    std::sort(latencies.begin(), latencies.end());
    std::printf("Recall@%d: %.1f%%\n", K, 100.0 * total_recall / Q);
    std::printf("Search P50: %.0f us\n", latencies[Q * 50 / 100]);
    std::printf("Search P90: %.0f us\n", latencies[Q * 90 / 100]);
    std::printf("Search P99: %.0f us\n", latencies[Q * 99 / 100]);
    std::printf("Search QPS: %.0f\n\n", 1e6 / latencies[Q * 50 / 100]);

    storage.sync();
    VectorStorage loaded({.data_path = "demo.db", .dimension = D});
    std::printf("Persistence: recovered %ld vectors\n", loaded.count());

    bool pass = (correct >= N * 0.99) && (total_recall / Q > 0.90f);
    std::printf("\nResult: %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
