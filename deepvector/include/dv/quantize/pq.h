#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "dv/types.h"

namespace dv {
namespace quantize {

// Product Quantization: splits vectors into M subspaces, each represented by one of K centroids.
// Memory: original dim*4 bytes 鈫?M bytes (e.g., 768*4=3072 鈫?96 bytes = 32x compression).
class ProductQuantizer {
public:
    // M: number of sub-quantizers (subspaces). dim must be divisible by M.
    // K: number of centroids per subspace (must be power-of-2, max 256 for uint8 codes).
    ProductQuantizer(Dimension dim, size_t M = 0, size_t K = 256);

    size_t M() const { return M_; }
    size_t K() const { return K_; }
    size_t dim() const { return dim_; }
    size_t dsub() const { return dsub_; }

    // Train on a set of vectors (n rows, dim columns, row-major).
    // Uses k-means++ initialization + Lloyd iteration for each subspace.
    void train(const float* vectors, size_t n, int max_iters = 25);

    // Encode a vector to M bytes (one per subspace)
    void encode(const float* vector, uint8_t* codes) const;

    // Decode to approximate reconstruction
    void decode(const uint8_t* codes, float* vector) const;

    // Precompute distance table for asymmetric distance computation (ADC).
    // For each subspace i 鈭?[0, M) and each centroid j 鈭?[0, K):
    //   dist_table[i * K + j] = L2_squared(query_sub_i, centroid_i_j)
    void computeDistanceTable(const float* query, float* dist_table) const;

    // Symmetric distance between two encoded vectors
    float symmetricDistance(const uint8_t* codes_a, const uint8_t* codes_b) const;

    // Asymmetric distance using precomputed table
    float asymmetricDistance(const uint8_t* codes, const float* dist_table) const;

    // Bulk asymmetric distance computation for a query and many codes
    void batchADC(const float* query, const uint8_t* codes, size_t n,
                  float* distances) const;

    bool isTrained() const { return trained_; }

    // Serialization
    void save(const std::string& path) const;
    static std::unique_ptr<ProductQuantizer> load(const std::string& path);

private:
    Dimension dim_;
    size_t M_;
    size_t K_;
    size_t dsub_;
    bool trained_;
    std::vector<float> centroids_;  // M * K * dsub_ floats
};

} // namespace quantize
} // namespace dv
