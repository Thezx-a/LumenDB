#pragma once
#include <cstdint>
#include <vector>
#include "lumendb/types.h"

namespace lumendb {
namespace quantize {

class ScalarQuantizer {
public:
    explicit ScalarQuantizer(Dimension dim);
    void train(const float* vectors, size_t n);

    void encode(const float* vector, int8_t* codes) const;
    void decode(const int8_t* codes, float* vector) const;
    float l2SquaredDistance(const int8_t* a, const int8_t* b) const;

    bool isTrained() const { return trained_; }
    Dimension dim() const { return dim_; }

private:
    Dimension dim_;
    bool trained_;
    std::vector<float> min_val_;
    std::vector<float> scale_;
};

} // namespace quantize
} // namespace lumendb
