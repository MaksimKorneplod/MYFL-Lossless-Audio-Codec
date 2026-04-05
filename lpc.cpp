#include "lpc.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<float> calculateLPCoefficients(const std::vector<int16_t>& samples, int order) {
    std::vector<float> lpc(order, 0.0f);
    if (samples.size() < static_cast<size_t>(order * 2)) return lpc;

    // Автокорреляция с оконной функцией Ханна
    std::vector<float> autocorr(order + 1, 0.0f);
    for (int i = 0; i <= order; ++i) {
        for (size_t j = 0; j < samples.size() - i; ++j) {
            float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * j / (samples.size() - 1)));
            autocorr[i] += static_cast<float>(samples[j]) * static_cast<float>(samples[j + i]) * window;
        }
    }

    // Проверка на слишком малую энергию
    if (std::abs(autocorr[0]) < 1e-5f) return lpc;

    // Нормализация
    for (int i = 1; i <= order; ++i) {
        autocorr[i] /= autocorr[0];
    }
    autocorr[0] = 1.0f;

    // Левинсон-Дурбин
    std::vector<float> error(order + 1, 0.0f);
    error[0] = autocorr[0];
    std::vector<float> prevLpc(order, 0.0f);

    for (int k = 0; k < order; ++k) {
        float lambda = -autocorr[k + 1];
        for (int i = 0; i < k; ++i) {
            lambda -= lpc[i] * autocorr[k - i];
        }

        if (std::abs(error[k]) < 1e-5f) break;
        lambda /= error[k];

        // Обновление коэффициентов с копией предыдущих
        prevLpc = lpc;
        lpc[k] = lambda;

        for (int i = 0; i < k; ++i) {
            lpc[i] = prevLpc[i] + lambda * prevLpc[k - 1 - i];
        }

        // Обновление ошибки
        error[k + 1] = error[k] * (1.0f - lambda * lambda);
    }

    return lpc;
}

std::vector<int> linearPredict(const std::vector<int16_t>& samples,
                               const float* coeffs, int order) {
    std::vector<int32_t> residuals(samples.size(), 0);

    if (samples.size() < static_cast<size_t>(order)) {
        return std::vector<int>(residuals.begin(), residuals.end());
    }

    // Первые order элементов — без предсказания
    for (int i = 0; i < order; ++i) {
        residuals[i] = samples[i];
    }

    // Остальные — через предсказание
    for (size_t i = order; i < samples.size(); ++i) {
        float predicted = 0.0f;
        for (int j = 0; j < order; ++j) {
            predicted += static_cast<float>(samples[i - j - 1]) * coeffs[j];
        }
        residuals[i] = samples[i] - static_cast<int16_t>(std::round(predicted));
    }

    return std::vector<int>(residuals.begin(), residuals.end());
}
