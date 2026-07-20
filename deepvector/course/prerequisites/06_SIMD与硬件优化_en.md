# SIMD and Hardware Optimization

> Modern CPUs provide SIMD instruction sets to accelerate vector operations. This document covers AVX2 basics.

## What is SIMD?

**SIMD** (Single Instruction Multiple Data): A single instruction processes multiple data elements simultaneously.

```
Scalar:  a[0] + b[0]  →  c[0]   (1 operation)
         a[1] + b[1]  →  c[1]   (1 operation)
         ...                    (N operations total)

SIMD:    [a[0..7]] + [b[0..7]]  →  [c[0..7]]  (1 operation, processes 8 elements)
```

## AVX2 Registers

| Register | Width | float capacity | int32 capacity |
|----------|-------|----------------|----------------|
| YMM0-YMM15 | 256 bit | 8 × float | 8 × int32 |
| XMM0-XMM15 | 128 bit | 4 × float | 4 × int32 |

## SIMD Operations Used in This Tutorial

### Batch Distance Computation

```cpp
#include <immintrin.h>

float l2_distance_avx2(const float* a, const float* b, int dim) {
    __m256 sum = _mm256_setzero_ps();
    for (int i = 0; i < dim; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 diff = _mm256_sub_ps(va, vb);
        sum = _mm256_fmadd_ps(diff, diff, sum);  // fused multiply-add
    }
    // Horizontal sum
    alignas(32) float temp[8];
    _mm256_store_ps(temp, sum);
    float result = 0;
    for (int i = 0; i < 8; ++i) result += temp[i];
    return std::sqrt(result);
}
```

### Memory Alignment

| Load Instruction | Alignment Requirement | Speed |
|-----------------|----------------------|-------|
| `_mm256_load_ps` | 32-byte aligned | Fast |
| `_mm256_loadu_ps` | No alignment required | Slightly slower |

## Compiler Options

```bash
# Enable AVX2
-mavx2

# Enable FMA (fused multiply-add)
-mfma

# Auto-vectorization
-O3 -march=native
```

## Performance Comparison

| Operation | Scalar | AVX2 | Speedup |
|-----------|--------|------|---------|
| L2 distance (128-dim) | ~200ns | ~30ns | ~6.7× |
| Inner product (128-dim) | ~180ns | ~25ns | ~7.2× |
| Batch search (10K) | ~2ms | ~0.3ms | ~6.7× |

## Related Chapters

- Ch02 vector distance: [02_向量与距离度量](../ch02_vectors_distance/02_向量与距离度量_zh.md)
- Ch07 quantization compression: [07_向量量化压缩](../ch07_quantization/07_向量量化压缩_zh.md)
