#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// Type-erased distance function using std::function
// Accepts any callable with signature: float(const float*, const float*, int)
class DistanceIndex {
public:
    using DistanceFn = std::function<float(const float*, const float*, int)>;

    template <typename F>
    void SetDistance(F&& fn) {
        dist_ = std::forward<F>(fn);
    }

    float Compute(const float* a, const float* b, int dim) const {
        return dist_(a, b, dim);
    }

    bool HasDistance() const { return dist_ != nullptr; }

private:
    DistanceFn dist_;
};

// Demonstration of different callable types
namespace distance_functions {

// Free function
inline float l2_distance(const float* a, const float* b, int dim) {
    float sum = 0.0f;
    for (int i = 0; i < dim; ++i) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum);
}

inline float inner_product(const float* a, const float* b, int dim) {
    float sum = 0.0f;
    for (int i = 0; i < dim; ++i) sum += a[i] * b[i];
    return sum;
}

// Functor (struct with operator())
struct CosineDistance {
    float operator()(const float* a, const float* b, int dim) const {
        float dot = 0.0f, na = 0.0f, nb = 0.0f;
        for (int i = 0; i < dim; ++i) {
            dot += a[i] * b[i];
            na += a[i] * a[i];
            nb += b[i] * b[i];
        }
        float denom = std::sqrt(na) * std::sqrt(nb);
        return (denom > 0.0f) ? (1.0f - dot / denom) : 0.0f;
    }
};

} // namespace distance_functions
