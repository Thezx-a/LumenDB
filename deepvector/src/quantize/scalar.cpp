#include "dv/quantize/scalar.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace dv {
namespace quantize {

ScalarQuantizer::ScalarQuantizer(Dimension dim)
    : dim_(dim), trained_(false) {
    min_val_.resize(dim, 0.0f);
    scale_.resize(dim, 0.0f);
}

void ScalarQuantizer::train(const float* vectors, size_t n) {
    if (n == 0) return;
    std::vector<float> mins(dim_, std::numeric_limits<float>::max());
    std::vector<float> maxs(dim_, std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < n; ++i) {
        for (Dimension d = 0; d < dim_; ++d) {
            float val = vectors[i * dim_ + d];
            if (val < mins[d]) mins[d] = val;
            if (val > maxs[d]) maxs[d] = val;
        }
    }

    for (Dimension d = 0; d < dim_; ++d) {
        min_val_[d] = mins[d];
        float range = maxs[d] - mins[d];
        scale_[d] = (range > 1e-8f) ? (range / 255.0f) : 1.0f;
    }
    trained_ = true;
}

void ScalarQuantizer::encode(const float* vector, int8_t* codes) const {
    for (Dimension d = 0; d < dim_; ++d) {
        float val = (vector[d] - min_val_[d]) / scale_[d];
        int ival = static_cast<int>(std::round(val));
        codes[d] = static_cast<int8_t>(std::max(-128, std::min(127, ival)));
    }
}

void ScalarQuantizer::decode(const int8_t* codes, float* vector) const {
    for (Dimension d = 0; d < dim_; ++d) {
        vector[d] = min_val_[d] + static_cast<float>(codes[d]) * scale_[d];
    }
}

float ScalarQuantizer::l2SquaredDistance(const int8_t* a, const int8_t* b) const {
    float dist = 0;
    for (Dimension d = 0; d < dim_; ++d) {
        float diff = static_cast<float>(a[d] - b[d]) * scale_[d];
        dist += diff * diff;
    }
    return dist;
}

} // namespace quantize
} // namespace dv
