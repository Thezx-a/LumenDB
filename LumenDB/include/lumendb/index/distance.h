#pragma once
#include <cmath>
#include <cstddef>
#include "../types.h"

#ifdef __AVX2__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

namespace lumendb {
namespace index {

namespace detail {

// Fallback: scalar L2 distance squared
inline float l2_squared_scalar(const float* a, const float* b, size_t dim) {
    float sum = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

// Fallback: scalar inner product (negated for minimizing)
inline float ip_scalar(const float* a, const float* b, size_t dim) {
    float sum = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        sum += a[i] * b[i];
    }
    return -sum;  // negate so smaller = better
}

// Fallback: cosine distance (1 - cosine similarity)
inline float cosine_scalar(const float* a, const float* b, size_t dim) {
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    if (na == 0.0f || nb == 0.0f) return 1.0f;
    return 1.0f - dot / std::sqrt(na * nb);
}

#ifdef __AVX2__
inline float l2_squared_avx2(const float* a, const float* b, size_t dim) {
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum = _mm256_fmadd_ps(diff, diff, sum);
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    float total = result[0] + result[1] + result[2] + result[3]
                + result[4] + result[5] + result[6] + result[7];
    for (; i < dim; ++i) {
        float diff = a[i] - b[i];
        total += diff * diff;
    }
    return total;
}

inline float ip_avx2(const float* a, const float* b, size_t dim) {
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        sum = _mm256_fmadd_ps(va, vb, sum);
    }
    float result[8];
    _mm256_storeu_ps(result, sum);
    float total = result[0] + result[1] + result[2] + result[3]
                + result[4] + result[5] + result[6] + result[7];
    for (; i < dim; ++i) total += a[i] * b[i];
    return -total;
}
#endif // __AVX2__

} // namespace detail

// Public API - dispatches to best available implementation

inline float l2_distance(const float* a, const float* b, size_t dim) {
#ifdef __AVX2__
    return std::sqrt(detail::l2_squared_avx2(a, b, dim));
#else
    return std::sqrt(detail::l2_squared_scalar(a, b, dim));
#endif
}

inline float l2_squared(const float* a, const float* b, size_t dim) {
#ifdef __AVX2__
    return detail::l2_squared_avx2(a, b, dim);
#else
    return detail::l2_squared_scalar(a, b, dim);
#endif
}

inline float inner_product(const float* a, const float* b, size_t dim) {
#ifdef __AVX2__
    return detail::ip_avx2(a, b, dim);
#else
    return detail::ip_scalar(a, b, dim);
#endif
}

inline float cosine_distance(const float* a, const float* b, size_t dim) {
    // Cosine isn't easily SIMD-accelerated due to the norm division
    return detail::cosine_scalar(a, b, dim);
}

// Generic distance calculator that dispatches based on metric
inline float compute_distance(const float* a, const float* b, size_t dim, DistanceMetric metric) {
    switch (metric) {
        case DistanceMetric::L2:
            return l2_squared(a, b, dim);  // Skip sqrt for comparison
        case DistanceMetric::InnerProduct:
            return inner_product(a, b, dim);
        case DistanceMetric::Cosine:
            return cosine_distance(a, b, dim);
    }
    return 0.0f;
}

} // namespace index
} // namespace lumendb
