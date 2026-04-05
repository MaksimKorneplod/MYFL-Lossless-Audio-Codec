#ifndef MYFL_TYPES_HPP
#define MYFL_TYPES_HPP

#include <vector>
#include <cstdint>
#include <sndfile.h>

// Константы для кодирования
const int PREDICTOR_ORDER = 2;

// Структура для аудиоданных
struct AudioData {
    std::vector<float> samples;
    SF_INFO info;
};

#pragma pack(push, 1)

struct MYFLHeader {
    char magic[4] = {'M','Y','F','L'};
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
    uint32_t totalSamples;
    uint32_t numBlocks;
};

struct BlockHeader {
    uint16_t param;        // Параметр Райса
    uint16_t sample_count; // Количество сэмплов
    uint32_t encoded_size; // Размер закодированных данных
};

#pragma pack(pop)

#endif // MYFL_TYPES_HPP
