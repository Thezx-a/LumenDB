# SIMD 与硬件优化

> 现代 CPU 提供 SIMD 指令集加速向量运算。本文档覆盖 AVX2 基础。

## 什么是 SIMD？

**SIMD**（Single Instruction Multiple Data）：一条指令同时处理多个数据。

```
标量：  a[0] + b[0]  →  c[0]   (1 次运算)
        a[1] + b[1]  →  c[1]   (1 次运算)
        ...                    (共 N 次)

SIMD：  [a[0..7]] + [b[0..7]]  →  [c[0..7]]  (1 次运算，处理 8 个)
```

## AVX2 寄存器

| 寄存器 | 宽度 | float 容量 | int32 容量 |
|--------|------|-----------|-----------|
| YMM0-YMM15 | 256 bit | 8 × float | 8 × int32 |
| XMM0-XMM15 | 128 bit | 4 × float | 4 × int32 |

## 本教程使用的 SIMD 操作

### 批量距离计算

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
    // 水平求和
    alignas(32) float temp[8];
    _mm256_store_ps(temp, sum);
    float result = 0;
    for (int i = 0; i < 8; ++i) result += temp[i];
    return std::sqrt(result);
}
```

### 内存对齐

| 加载指令 | 对齐要求 | 速度 |
|----------|----------|------|
| `_mm256_load_ps` | 32 字节对齐 | 快 |
| `_mm256_loadu_ps` | 无对齐要求 | 稍慢 |

## 编译器选项

```bash
# 启用 AVX2
-mavx2

# 启用 FMA（融合乘加）
-mfma

# 自动向量化
-O3 -march=native
```

## 性能对比

| 操作 | 标量 | AVX2 | 加速比 |
|------|------|------|--------|
| L2 距离 (128-dim) | ~200ns | ~30ns | ~6.7× |
| 内积 (128-dim) | ~180ns | ~25ns | ~7.2× |
| 批量搜索 (10K) | ~2ms | ~0.3ms | ~6.7× |

## 相关章节

- Ch02 向量距离：[02_向量与距离度量](../ch02_vectors_distance/02_向量与距离度量_zh.md)
- Ch07 量化压缩：[07_向量量化压缩](../ch07_quantization/07_向量量化压缩_zh.md)
