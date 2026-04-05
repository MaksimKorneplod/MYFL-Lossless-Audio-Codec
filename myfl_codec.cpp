#include "myfl_codec.hpp"
#include "myfl_types.hpp"
#include "lpc.hpp"
#include "rice.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <limits>

void saveAsMYFL(const std::vector<float>& samples, const std::string& filename,
                int sampleRate, int channels) {
    if (samples.empty()) {
        throw std::runtime_error("Empty input samples");
    }

    // Квантование в 16-bit
    std::vector<int16_t> quantized(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        quantized[i] = static_cast<int32_t>(
            std::round(std::clamp(samples[i] * 32767.0f, -32768.0f, 32767.0f))
        );
    }

    const size_t BLOCK_SIZE = 1024;

    MYFLHeader header{};
    header.sampleRate = sampleRate;
    header.channels = channels;
    header.bitsPerSample = 16;
    header.totalSamples = samples.size() / channels;
    header.numBlocks = (quantized.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Cannot create output file");
    }

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    std::cout << "Encoding MYFL: 0%" << std::flush;

    for (size_t block_start = 0; block_start < quantized.size(); block_start += BLOCK_SIZE) {
        size_t block_end = std::min(block_start + BLOCK_SIZE, quantized.size());
        std::vector<int16_t> block_samples(quantized.begin() + block_start,
                                          quantized.begin() + block_end);

        auto coeffs = calculateLPCoefficients(block_samples, PREDICTOR_ORDER);
        auto residuals = linearPredict(block_samples, coeffs.data(), PREDICTOR_ORDER);

        if (residuals.size() != block_samples.size()) {
            throw std::runtime_error("Residuals count mismatch");
        }

        // Поиск оптимального параметра Райса
        int best_param = 0;
        size_t best_size = std::numeric_limits<size_t>::max();

        for (int param = 1; param <= 15; ++param) {
            size_t estimated_size = estimateRiceSize(residuals, param);
            if (estimated_size < best_size) {
                best_size = estimated_size;
                best_param = param;
            }
        }

        auto best_encoded = riceEncode(residuals, best_param);

        // Логирование
        std::cout << " Block " << block_start / BLOCK_SIZE
                  << ": Param=" << best_param
                  << ", Samples=" << block_samples.size()
                  << ", Encoded size=" << best_encoded.size()
                  << "\n";

        BlockHeader block_header{};
        block_header.param = best_param;
        block_header.sample_count = block_samples.size();
        block_header.encoded_size = best_encoded.size();

        out.write(reinterpret_cast<const char*>(&block_header), sizeof(block_header));

        // Запись LPC-коэффициентов
        for (float coeff : coeffs) {
            uint32_t u;
            std::memcpy(&u, &coeff, sizeof(coeff));
            out.write(reinterpret_cast<const char*>(&u), sizeof(u));
        }

        // Запись закодированных остатков
        out.write(reinterpret_cast<const char*>(best_encoded.data()), best_encoded.size());

        int progress = (100 * block_start) / quantized.size();
        std::cout << "\rEncoding MYFL: " << progress << "%" << std::flush;
    }

    std::cout << "\rEncoding MYFL: 100% - Done!" << std::endl;
}

std::vector<float> decodeMYFL(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open MYFL file");
    }

    MYFLHeader header;
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        throw std::runtime_error("Failed to read MYFL header");
    }

    std::vector<float> samples;
    samples.reserve(header.totalSamples * header.channels);

    for (size_t block = 0; block < header.numBlocks; ++block) {
        BlockHeader block_header;
        if (!in.read(reinterpret_cast<char*>(&block_header), sizeof(block_header))) {
            throw std::runtime_error("Failed to read block header");
        }

        // Чтение LPC-коэффициентов
        std::vector<float> coeffs(PREDICTOR_ORDER);
        for (size_t i = 0; i < coeffs.size(); ++i) {
            uint32_t u;
            if (!in.read(reinterpret_cast<char*>(&u), sizeof(u))) {
                throw std::runtime_error("Failed to read coefficient");
            }
            float f;
            std::memcpy(&f, &u, sizeof(f));
            coeffs[i] = f;
        }

        // Чтение закодированных остатков
        std::vector<uint8_t> encoded(block_header.encoded_size);
        if (!in.read(reinterpret_cast<char*>(encoded.data()), encoded.size())) {
            throw std::runtime_error("Failed to read encoded residuals");
        }

        auto residuals = riceDecode(encoded, block_header.param);

        if (residuals.size() != block_header.sample_count) {
            residuals.resize(block_header.sample_count);
        }

        std::vector<int32_t> block_samples(block_header.sample_count);

        // Первые PREDICTOR_ORDER сэмплов — напрямую
        for (int i = 0; i < PREDICTOR_ORDER && i < block_samples.size(); ++i) {
            block_samples[i] = residuals[i];
        }

        // Остальные — через предсказание
        for (size_t i = PREDICTOR_ORDER; i < block_samples.size(); ++i) {
            float predicted = 0.0f;
            for (int j = 0; j < PREDICTOR_ORDER; ++j) {
                predicted += block_samples[i - j - 1] * coeffs[j];
            }
            block_samples[i] = residuals[i] + static_cast<int32_t>(std::round(predicted));
        }

        // Нормализация и добавление
        for (auto sample : block_samples) {
            samples.push_back(std::clamp(sample / 32768.0f, -1.0f, 1.0f));
        }
    }

    if (samples.size() != header.totalSamples * header.channels) {
        std::cerr << "Warning: Decoded samples count doesn't match header" << std::endl;
    }

    return samples;
}
