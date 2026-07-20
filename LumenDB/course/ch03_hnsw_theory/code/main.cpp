#include "hnsw.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <unordered_set>

int main() {
    const int DIM = 128;
    const int N = 1000;
    const int M = 16;
    const int M_max = 32;
    const int ef_search = 100;

    NSWIndex idx(M, M_max, ef_search, DIM);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    std::vector<std::vector<float>> data(N);
    for (int i = 0; i < N; i++) {
        data[i].resize(DIM);
        for (int j = 0; j < DIM; j++)
            data[i][j] = dist(rng);
        idx.insert(data[i].data());
    }

    idx.print_stats();

    std::vector<float> query(DIM);
    for (int j = 0; j < DIM; j++)
        query[j] = dist(rng);

    auto results = idx.knn_search(query.data(), 5);
    std::cout << "\nTop-5 results:" << std::endl;
    for (auto& [d, id] : results) {
        std::cout << "  id=" << id << " dist=" << d << std::endl;
    }

    std::vector<std::pair<float, int>> brute(N);
    for (int i = 0; i < N; i++) {
        brute[i] = {idx.l2_distance(query.data(), data[i].data()), i};
    }
    std::sort(brute.begin(), brute.end());

    int matches = 0;
    std::unordered_set<int> result_ids;
    for (auto& [d, id] : results) result_ids.insert(id);
    for (int i = 0; i < 5; i++) {
        if (result_ids.count(brute[i].second)) matches++;
    }
    std::cout << "\nRecall@5: " << matches << "/5 = "
              << (matches / 5.0 * 100) << "%" << std::endl;

    return 0;
}
