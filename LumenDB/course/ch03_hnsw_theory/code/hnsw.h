#pragma once

#include <vector>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

struct Node {
    int id;
    std::vector<float> vec;
    std::vector<int> neighbors;
};

class NSWIndex {
public:
    int M;
    int M_max;
    int ef_search;
    int dim;
    std::vector<Node> nodes;
    int entry_point = -1;

    NSWIndex(int M_, int M_max_, int ef_s, int dim_)
        : M(M_), M_max(M_max_), ef_search(ef_s), dim(dim_) {}

    float l2_distance(const float* a, const float* b) const {
        float sum = 0.0f;
        for (int i = 0; i < dim; i++) {
            float d = a[i] - b[i];
            sum += d * d;
        }
        return std::sqrt(sum);
    }

    std::vector<std::pair<float, int>> search_layer(
        const float* query, int ep, int ef) const
    {
        auto cmp = [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
            return a.first < b.first;
        };
        std::priority_queue<std::pair<float, int>,
            std::vector<std::pair<float, int>>, decltype(cmp)> candidates(cmp);

        auto cmp_min = [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
            return a.first > b.first;
        };
        std::priority_queue<std::pair<float, int>,
            std::vector<std::pair<float, int>>, decltype(cmp_min)> results(cmp_min);

        std::unordered_set<int> visited;

        float d = l2_distance(query, nodes[ep].vec.data());
        candidates.push({d, ep});
        results.push({d, ep});
        visited.insert(ep);

        while (!candidates.empty()) {
            auto [dist_c, c] = candidates.top(); candidates.pop();
            auto [dist_f, f] = results.top();
            if (dist_c > dist_f) break;

            for (int n : nodes[c].neighbors) {
                if (visited.count(n)) continue;
                visited.insert(n);

                float dist = l2_distance(query, nodes[n].vec.data());
                if (results.size() < (size_t)ef || dist < results.top().first) {
                    candidates.push({dist, n});
                    results.push({dist, n});
                    if (results.size() > (size_t)ef)
                        results.pop();
                }
            }
        }

        std::vector<std::pair<float, int>> out;
        while (!results.empty()) {
            out.push_back(results.top());
            results.pop();
        }
        std::reverse(out.begin(), out.end());
        return out;
    }

    std::vector<int> select_neighbors_simple(
        const float* query,
        const std::vector<std::pair<float, int>>& candidates,
        int M_sel) const
    {
        std::vector<int> result;
        for (size_t i = 0; i < candidates.size() && result.size() < (size_t)M_sel; i++) {
            result.push_back(candidates[i].second);
        }
        return result;
    }

    void insert(const float* vec) {
        int new_id = nodes.size();
        nodes.push_back({new_id,
            std::vector<float>(vec, vec + dim), {}});

        if (new_id == 0) {
            entry_point = 0;
            return;
        }

        auto candidates = search_layer(vec, entry_point, ef_search);
        auto sel = select_neighbors_simple(vec, candidates, M);

        for (int nid : sel) {
            nodes[new_id].neighbors.push_back(nid);
            if (nodes[nid].neighbors.size() < (size_t)M_max) {
                nodes[nid].neighbors.push_back(new_id);
            }
        }
    }

    std::vector<std::pair<float, int>> knn_search(const float* query, int k) {
        if (nodes.empty()) return {};
        auto results = search_layer(query, entry_point, std::max(ef_search, k));
        results.resize(std::min((int)results.size(), k));
        return results;
    }

    void print_stats() const {
        std::cout << "Nodes: " << nodes.size() << std::endl;
        int total_edges = 0;
        for (auto& n : nodes) total_edges += n.neighbors.size();
        std::cout << "Total edges: " << total_edges << std::endl;
        std::cout << "Avg degree: "
                  << (nodes.empty() ? 0.0 : (double)total_edges / nodes.size())
                  << std::endl;
    }
};
