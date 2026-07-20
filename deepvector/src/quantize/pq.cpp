#include "dv/quantize/pq.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>

namespace dv {
namespace quantize {

static void kmeans(const float* data, size_t n, size_t d, size_t K,
                   float* centroids, int max_iters) {
    // Random initialization
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    for (size_t k = 0; k < K; ++k) {
        size_t idx = dist(rng);
        std::memcpy(centroids + k * d, data + idx * d, d * sizeof(float));
    }

    std::vector<int> assignments(n, 0);
    std::vector<int> counts(K, 0);

    for (int iter = 0; iter < max_iters; ++iter) {
        // Assign each vector to nearest centroid
        bool changed = false;
        for (size_t i = 0; i < n; ++i) {
            const float* vec = data + i * d;
            float bestDist = std::numeric_limits<float>::max();
            int bestK = 0;
            for (size_t k = 0; k < K; ++k) {
                float dist = 0;
                for (size_t j = 0; j < d; ++j) {
                    float diff = vec[j] - centroids[k * d + j];
                    dist += diff * diff;
                }
                if (dist < bestDist) { bestDist = dist; bestK = static_cast<int>(k); }
            }
            if (assignments[i] != bestK) { assignments[i] = bestK; changed = true; }
        }
        if (!changed) break;

        // Update centroids
        std::fill(counts.begin(), counts.end(), 0);
        std::fill(centroids, centroids + K * d, 0.0f);
        for (size_t i = 0; i < n; ++i) {
            int k = assignments[i];
            counts[k]++;
            const float* vec = data + i * d;
            for (size_t j = 0; j < d; ++j) centroids[k * d + j] += vec[j];
        }
        for (size_t k = 0; k < K; ++k) {
            if (counts[k] > 0) {
                for (size_t j = 0; j < d; ++j) centroids[k * d + j] /= counts[k];
            }
        }
    }
}

ProductQuantizer::ProductQuantizer(Dimension dim, size_t M, size_t K)
    : dim_(dim), M_(M ? M : dim / 4), K_(K), trained_(false) {
    if (dim_ % M_ != 0) throw std::runtime_error("dim must be divisible by M");
    if (K_ > 256 || (K_ & (K_ - 1)) != 0) throw std::runtime_error("K must be power-of-2 and 鈮?256");
    dsub_ = dim_ / M_;
    centroids_.resize(M_ * K_ * dsub_, 0.0f);
}

void ProductQuantizer::train(const float* vectors, size_t n, int max_iters) {
    if (n < K_) throw std::runtime_error("Not enough training vectors");

    // Train each subspace independently
    std::vector<float> subVectors(n * dsub_);
    for (size_t m = 0; m < M_; ++m) {
        // Extract subspace m from all vectors
        for (size_t i = 0; i < n; ++i) {
            std::memcpy(subVectors.data() + i * dsub_,
                       vectors + i * dim_ + m * dsub_,
                       dsub_ * sizeof(float));
        }
        // Run k-means on this subspace
        kmeans(subVectors.data(), n, dsub_, K_,
               centroids_.data() + m * K_ * dsub_, max_iters);
    }
    trained_ = true;
}

void ProductQuantizer::encode(const float* vector, uint8_t* codes) const {
    for (size_t m = 0; m < M_; ++m) {
        const float* sub = vector + m * dsub_;
        const float* cb = centroids_.data() + m * K_ * dsub_;
        float bestDist = std::numeric_limits<float>::max();
        uint8_t bestK = 0;
        for (size_t k = 0; k < K_; ++k) {
            float dist = 0;
            for (size_t j = 0; j < dsub_; ++j) {
                float diff = sub[j] - cb[k * dsub_ + j];
                dist += diff * diff;
            }
            if (dist < bestDist) { bestDist = dist; bestK = static_cast<uint8_t>(k); }
        }
        codes[m] = bestK;
    }
}

void ProductQuantizer::decode(const uint8_t* codes, float* vector) const {
    for (size_t m = 0; m < M_; ++m) {
        const float* cb = centroids_.data() + m * K_ * dsub_;
        std::memcpy(vector + m * dsub_, cb + codes[m] * dsub_, dsub_ * sizeof(float));
    }
}

void ProductQuantizer::computeDistanceTable(const float* query, float* dist_table) const {
    for (size_t m = 0; m < M_; ++m) {
        const float* subQuery = query + m * dsub_;
        const float* cb = centroids_.data() + m * K_ * dsub_;
        for (size_t k = 0; k < K_; ++k) {
            float dist = 0;
            for (size_t j = 0; j < dsub_; ++j) {
                float diff = subQuery[j] - cb[k * dsub_ + j];
                dist += diff * diff;
            }
            dist_table[m * K_ + k] = dist;
        }
    }
}

float ProductQuantizer::symmetricDistance(const uint8_t* codes_a, const uint8_t* codes_b) const {
    float dist = 0;
    for (size_t m = 0; m < M_; ++m) {
        const float* cb = centroids_.data() + m * K_ * dsub_;
        const float* ca = cb + codes_a[m] * dsub_;
        const float* cb_sub = cb + codes_b[m] * dsub_;
        for (size_t j = 0; j < dsub_; ++j) {
            float diff = ca[j] - cb_sub[j];
            dist += diff * diff;
        }
    }
    return dist;
}

float ProductQuantizer::asymmetricDistance(const uint8_t* codes, const float* dist_table) const {
    float dist = 0;
    for (size_t m = 0; m < M_; ++m) {
        dist += dist_table[m * K_ + codes[m]];
    }
    return dist;
}

void ProductQuantizer::batchADC(const float* query, const uint8_t* codes, size_t n,
                                 float* distances) const {
    std::vector<float> dist_table(M_ * K_);
    computeDistanceTable(query, dist_table.data());
    for (size_t i = 0; i < n; ++i) {
        distances[i] = asymmetricDistance(codes + i * M_, dist_table.data());
    }
}

void ProductQuantizer::save(const std::string& path) const {
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&dim_), sizeof(dim_));
    ofs.write(reinterpret_cast<const char*>(&M_), sizeof(M_));
    ofs.write(reinterpret_cast<const char*>(&K_), sizeof(K_));
    ofs.write(reinterpret_cast<const char*>(&trained_), sizeof(trained_));
    size_t sz = centroids_.size();
    ofs.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    ofs.write(reinterpret_cast<const char*>(centroids_.data()), sz * sizeof(float));
}

std::unique_ptr<ProductQuantizer> ProductQuantizer::load(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return nullptr;
    Dimension dim;
    size_t M, K, sz;
    bool trained;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    ifs.read(reinterpret_cast<char*>(&M), sizeof(M));
    ifs.read(reinterpret_cast<char*>(&K), sizeof(K));
    ifs.read(reinterpret_cast<char*>(&trained), sizeof(trained));
    ifs.read(reinterpret_cast<char*>(&sz), sizeof(sz));
    auto pq = std::unique_ptr<ProductQuantizer>(new ProductQuantizer(dim, M, K));
    pq->centroids_.resize(sz);
    ifs.read(reinterpret_cast<char*>(pq->centroids_.data()), sz * sizeof(float));
    pq->trained_ = trained;
    return pq;
}

} // namespace quantize
} // namespace dv
