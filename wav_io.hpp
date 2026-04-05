#ifndef WAV_IO_HPP
#define WAV_IO_HPP

#include "myfl_types.hpp"

#include <string>

/**
 * Загружает WAV-файл через libsndfile.
 */
AudioData loadAudio(const char* filename);

/**
 * Сохраняет float-сэмплы в WAV-файл заданного формата.
 */
void saveAsWav(const std::vector<float>& samples, const std::string& filename,
               int sampleRate, int channels, int format);

#endif // WAV_IO_HPP
