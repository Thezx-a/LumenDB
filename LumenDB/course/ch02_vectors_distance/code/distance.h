#pragma once

#include <cstddef>

float l2_squared(const float* a, const float* b, size_t dim);
float ip_distance(const float* a, const float* b, size_t dim);
float cosine_distance(const float* a, const float* b, size_t dim);
float ip_distance_avx2(const float* a, const float* b, size_t dim);
