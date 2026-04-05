#ifndef RICE_HPP
#define RICE_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

/**
 * ZigZag-кодирование: преобразует знаковое целое в беззнаковое,
 * чтобы малые по модулю значения стали малыми положительными.
 */
inline unsigned int zigZagEncode(int val) {
    return (static_cast<unsigned int>(val) << 1) ^ (val >> 31);
}

/**
 * ZigZag-декодирование: обратное преобразование.
 */
inline int zigZagDecode(unsigned int z) {
    return static_cast<int>((z >> 1) ^ -(z & 1));
}

/**
 * Кодирование остатков методом Райса.
 * param — параметр k ∈ [1, 15].
 */
std::vector<uint8_t> riceEncode(const std::vector<int>& residuals, int param);

/**
 * Декодирование Rice-потока обратно в остатки.
 */
std::vector<int> riceDecode(const std::vector<uint8_t>& encoded, int param);

/**
 * Оценка размера закодированных данных без реального кодирования.
 */
size_t estimateRiceSize(const std::vector<int32_t>& residuals, int param);

#endif // RICE_HPP
