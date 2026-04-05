#ifndef LPC_HPP
#define LPC_HPP

#include <vector>
#include <cstdint>

/**
 * Вычисляет коэффициенты линейного предсказания (LPC)
 * автокорреляционным методом с оконной функцией Ханна
 * и алгоритмом Левинсона-Дурбина.
 */
std::vector<float> calculateLPCoefficients(const std::vector<int16_t>& samples, int order);

/**
 * Линейное предсказание — вычисляет остатки
 * как разницу между реальными и предсказанными значениями.
 */
std::vector<int> linearPredict(const std::vector<int16_t>& samples,
                               const float* coeffs, int order);

#endif // LPC_HPP
