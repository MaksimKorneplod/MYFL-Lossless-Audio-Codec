#ifndef MYFL_CODEC_HPP
#define MYFL_CODEC_HPP

#include <vector>
#include <string>
#include <cstdint>

/**
 * Кодирует аудио-сэмплы в формат MYFL и сохраняет в файл.
 */
void saveAsMYFL(const std::vector<float>& samples, const std::string& filename,
                int sampleRate, int channels);

/**
 * Декодирует MYFL-файл и восстанавливает аудио-сэмплы.
 */
std::vector<float> decodeMYFL(const std::string& filename);

#endif // MYFL_CODEC_HPP
