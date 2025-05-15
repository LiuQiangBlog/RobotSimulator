//
// Created by liuqiang on 25-5-15.
//
#include <immintrin.h>
#include <iostream>
#include <vector>

#include <immintrin.h>
#include <iostream>
#include <vector>

// 通用矩阵乘法实现
std::vector<std::vector<float>> genericMatrixMultiply(const std::vector<std::vector<float>>& a, const std::vector<std::vector<float>>& b) {
    std::vector<std::vector<float>> c(a.size(), std::vector<float>(a.size(), 0));
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < a.size(); ++j) {
            for (size_t k = 0; k < a.size(); ++k) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return c;
}

// SIMD 矩阵乘法实现
std::vector<std::vector<float>> simdMatrixMultiply(const std::vector<std::vector<float>>& a, const std::vector<std::vector<float>>& b) {
    std::vector<std::vector<float>> c(a.size(), std::vector<float>(a.size(), 0));
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < a.size(); ++j) {
            __m256 result = _mm256_setzero_ps();
            for (size_t k = 0; k < a.size(); k += 8) {
                __m256 a_line = _mm256_loadu_ps(&a[i][k]);
                __m256 b_line = _mm256_loadu_ps(&b[j][k]);
                result = _mm256_add_ps(result, _mm256_mul_ps(a_line, b_line));
                result = _mm256_hadd_ps(result, result);
                result = _mm256_hadd_ps(result, result);
                result[1] = result[4];
                result = _mm256_hadd_ps(result, result);
            }
            c[i][j] = result[0];
        }
    }
    return c;
}

// 根据架构选择合适的矩阵乘法函数
std::vector<std::vector<float>> matrixMultiply(const std::vector<std::vector<float>>& a, const std::vector<std::vector<float>>& b) {
#ifdef __x86_64__
    return simdMatrixMultiply(a, b);
#else
    return genericMatrixMultiply(a, b);
#endif
}

int main()
{
    // 示例4x4矩阵
    std::vector<std::vector<float>> T = {
        {1.0, 0.0, 0.0, 1.0},
        {0.0, 1.0, 0.0, 2.0},
        {0.0, 0.0, 1.0, 3.0},
        {0.0, 0.0, 0.0, 1.0}
    };

    // 示例4x1向量（齐次坐标）
    Vector4 P = {1.0, 2.0, 3.0, 1.0};

    // 进行矩阵向量乘法
    Vector4 result = mmFloating(T, P);

    // 输出结果
    printVector(result, "Result:");

    return 0;
}