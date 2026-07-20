#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <random>
#include <limits>
#include <numeric>

class ProductQuantizer {
public:
    ProductQuantizer(int D, int M)
        : D_(D), M_(M), dsub_(D / M), K_(256) {
        codebooks_.resize(M_, std::vector<std::vector<float>>(K_, std::vector<float>(dsub_, 0.0f)));
    }

    // Train codebooks on N vectors of dimension D
    void Train(const float* vectors, size_t N, int max_iters = 25) {
        for (int m = 0; m < M_; ++m) {
            // Extract subvectors for subspace m
            std::vector<float> subvecs(N * dsub_);
            for (size_t i = 0; i < N; ++i) {
                std::copy(vectors + i * D_ + m * dsub_,
                          vectors + i * D_ + m * dsub_ + dsub_,
                          subvecs.begin() + i * dsub_);
            }
            // Run k-means on this subspace
            kmeans(subvecs.data(), N, dsub_, K_, codebooks_[m], max_iters);
        }
    }

    // Encode a single vector into M bytes (PQ codes)
    void Encode(const float* vec, uint8_t* codes) const {
        for (int m = 0; m < M_; ++m) {
            const float* subvec = vec + m * dsub_;
            float best_dist = std::numeric_limits<float>::max();
            int best_k = 0;
            for (int k = 0; k < K_; ++k) {
                float dist = l2_sqr(subvec, codebooks_[m][k].data(), dsub_);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_k = k;
                }
            }
            codes[m] = static_cast<uint8_t>(best_k);
        }
    }

    // Reconstruct a vector from PQ codes
    void Decode(const uint8_t* codes, float* out) const {
        for (int m = 0; m < M_; ++m) {
            std::copy(codebooks_[m][codes[m]].begin(),
                      codebooks_[m][codes[m]].end(),
                      out + m * dsub_);
        }
    }

    // Precompute distance table for ADC search: M x K matrix
    void PrecomputeDistTable(const float* query,
                             std::vector<std::vector<float>>& dist_table) const {
        dist_table.resize(M_, std::vector<float>(K_));
        for (int m = 0; m < M_; ++m) {
            const float* sub_q = query + m * dsub_;
            for (int k = 0; k < K_; ++k) {
                dist_table[m][k] = l2_sqr(sub_q, codebooks_[m][k].data(), dsub_);
            }
        }
    }

    // ADC search: find top-K nearest neighbors using PQ codes
    void ADCSearch(
        const float* query,
        const std::vector<std::vector<uint8_t>>& all_codes,
        int top_k,
        std::vector<std::pair<float, int>>& results
    ) const {
        // Build distance table
        std::vector<std::vector<float>> dist_table;
        PrecomputeDistTable(query, dist_table);

        // Scan all encoded vectors
        size_t N = all_codes.size();
        results.clear();
        results.reserve(N);

        for (size_t i = 0; i < N; ++i) {
            float dist = 0.0f;
            for (int m = 0; m < M_; ++m) {
                dist += dist_table[m][all_codes[i][m]];
            }
            results.push_back({dist, static_cast<int>(i)});
        }

        // Partial sort for top-K
        if (static_cast<int>(results.size()) > top_k) {
            std::partial_sort(results.begin(), results.begin() + top_k, results.end());
            results.resize(top_k);
        } else {
            std::sort(results.begin(), results.end());
        }
    }

    // Getters
    int D() const { return D_; }
    int M() const { return M_; }
    int K() const { return K_; }
    int dsub() const { return dsub_; }
    const std::vector<std::vector<std::vector<float>>>& Codebooks() const { return codebooks_; }

private:
    static float l2_sqr(const float* a, const float* b, int d) {
        float sum = 0.0f;
        for (int i = 0; i < d; ++i) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return sum;
    }

    static void kmeans(const float* data, size_t N, int D, int K,
                       std::vector<std::vector<float>>& centroids,
                       int max_iters) {
        // k-means++ initialization
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> unif(0.0f, 1.0f);

        centroids.resize(K, std::vector<float>(D));

        // First centroid: random
        size_t first = static_cast<size_t>(unif(rng) * N);
        std::copy(data + first * D, data + first * D + D, centroids[0].begin());

        std::vector<float> min_dists(N, std::numeric_limits<float>::max());
        for (int k = 1; k < K; ++k) {
            // Update min distances
            for (size_t i = 0; i < N; ++i) {
                float dist = l2_sqr(data + i * D, centroids[k - 1].data(), D);
                min_dists[i] = std::min(min_dists[i], dist);
            }
            // Weighted random selection
            float total = std::accumulate(min_dists.begin(), min_dists.end(), 0.0f);
            float r = unif(rng) * total;
            float cumulative = 0.0f;
            for (size_t i = 0; i < N; ++i) {
                cumulative += min_dists[i];
                if (cumulative >= r) {
                    std::copy(data + i * D, data + i * D + D, centroids[k].begin());
                    break;
                }
            }
        }

        // Lloyd iterations
        std::vector<int> assignments(N);
        for (int iter = 0; iter < max_iters; ++iter) {
            // Assignment
            for (size_t i = 0; i < N; ++i) {
                float best = std::numeric_limits<float>::max();
                int best_k = 0;
                for (int k = 0; k < K; ++k) {
                    float dist = l2_sqr(data + i * D, centroids[k].data(), D);
                    if (dist < best) {
                        best = dist;
                        best_k = k;
                    }
                }
                assignments[i] = best_k;
            }

            // Update
            std::vector<std::vector<float>> new_centroids(K, std::vector<float>(D, 0.0f));
            std::vector<int> counts(K, 0);
            for (size_t i = 0; i < N; ++i) {
                int c = assignments[i];
                counts[c]++;
                for (int d = 0; d < D; ++d) {
                    new_centroids[c][d] += data[i * D + d];
                }
            }
            for (int k = 0; k < K; ++k) {
                if (counts[k] > 0) {
                    for (int d = 0; d < D; ++d) {
                        new_centroids[k][d] /= counts[k];
                    }
                }
            }
            centroids = std::move(new_centroids);
        }
    }

    int D_, M_, dsub_, K_;
    std::vector<std::vector<std::vector<float>>> codebooks_; // [M][K][dsub]
};
