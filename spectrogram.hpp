#ifndef SPECTROGRAM_HPP
#define SPECTROGRAM_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <cstddef>

// Параметры FFT
const int FFT_SIZE = 1024;
const int HOP_SIZE = FFT_SIZE / 4;

/**
 * Инициализирует глобальные структуры FFTW.
 * Вызывать один раз при старте.
 */
void initFFTW();

/**
 * Освобождает ресурсы FFTW.
 * Вызывать при завершении.
 */
void cleanupFFTW();

/**
 * Вычисляет спектр сигнала через FFT.
 */
std::vector<float> computeSpectrum(const std::vector<float>& audio, size_t pos,
                                   int channel, int totalChannels);

/**
 * Отрисовывает спектрограмму в SFML-окне.
 */
void drawSpectrogram(sf::RenderWindow& window, const std::vector<float>& audio,
                     int sampleRate, int channels, const std::string& title);

#endif // SPECTROGRAM_HPP
