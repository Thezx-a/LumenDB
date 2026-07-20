#include "distance.h"
#include <cmath>
#include <immintrin.h>

float l2_squared(const float* a, const float* b, size_t dim) {
    float sum = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

float ip_distance(const float* a, const float* b, size_t dim) {
    float sum = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        sum += a[i] * b[i];
    }
    return -sum;
}

float cosine_distance(const float* a, const float* b, size_t dim) {
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-9f) return 1.0f;
    return 1.0f - dot / denom;
}

float ip_distance_avx2(const float* a, const float* b, size_t dim) {
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;

    for (; i + 8 <= dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        sum = _mm256_fmadd_ps(va, vb, sum);
    }

    float result = 0.0f;
    float buf[8];
    _mm256_storeu_ps(buf, sum);
    for (int j = 0; j < 8; j++) result += buf[j];

    for (; i < dim; i++) {
        result += a[i] * b[i];
    }

    return -result;
}
