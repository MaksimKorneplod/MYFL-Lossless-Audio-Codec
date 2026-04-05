#include "spectrogram.hpp"
#include <fftw3.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Глобальные переменные для FFTW
static fftw_complex* fftIn = nullptr;
static fftw_complex* fftOut = nullptr;
static fftw_plan fftPlan = nullptr;

void initFFTW() {
    fftIn = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftOut = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftPlan = fftw_plan_dft_1d(FFT_SIZE, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);
}

void cleanupFFTW() {
    if (fftPlan) fftw_destroy_plan(fftPlan);
    if (fftIn) fftw_free(fftIn);
    if (fftOut) fftw_free(fftOut);
}

std::vector<float> computeSpectrum(const std::vector<float>& audio, size_t pos,
                                   int channel, int totalChannels) {
    std::vector<float> window(FFT_SIZE, 0.0f);

    // Выборка данных для конкретного канала
    for (int i = 0; i < FFT_SIZE; ++i) {
        size_t sampleIdx = pos * totalChannels + i * totalChannels + channel;
        if (sampleIdx < audio.size()) {
            window[i] = audio[sampleIdx];
        }
    }

    // Оконная функция Ханна
    for (int i = 0; i < FFT_SIZE; ++i) {
        float hann = 0.5f * (1 - cos(2 * M_PI * i / (FFT_SIZE - 1)));
        window[i] *= hann;
    }

    // FFT
    for (int i = 0; i < FFT_SIZE; ++i) {
        fftIn[i][0] = window[i];
        fftIn[i][1] = 0.0;
    }

    fftw_execute(fftPlan);

    // Вычисление амплитудного спектра
    std::vector<float> spectrum(FFT_SIZE / 2);
    for (size_t i = 0; i < spectrum.size(); ++i) {
        spectrum[i] = std::sqrt(fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1]);
    }

    return spectrum;
}

void drawSpectrogram(sf::RenderWindow& window, const std::vector<float>& audio,
                     int sampleRate, int channels, const std::string& title) {
    const int width = window.getSize().x;
    const int height = window.getSize().y;

    window.clear(sf::Color::White);

    sf::Font font;
    if (font.openFromFile("arial.ttf")) {
        sf::Text text(font, title, 20);
        text.setFillColor(sf::Color::White);
        text.setPosition(sf::Vector2f(10, 10));
        window.draw(text);
    }

    // Отрисовка спектрограммы для каждого канала
    for (int ch = 0; ch < channels; ++ch) {
        for (int x = 0; x < width; ++x) {
            size_t framePos = x * HOP_SIZE;
            if (framePos * channels + FFT_SIZE * channels > audio.size()) break;

            auto spectrum = computeSpectrum(audio, framePos, ch, channels);

            for (int y = 30; y < height; ++y) {
                // Преобразование в логарифмическую шкалу
                float logY = std::powf(static_cast<float>(y - 30) / (height - 30), 2.0f);
                int freqBin = static_cast<int>(logY * (FFT_SIZE / 2));
                freqBin = std::min(freqBin, static_cast<int>(spectrum.size()) - 1);

                float magnitude = spectrum[freqBin];
                float db = 20 * std::log10f(magnitude + 1e-6f);
                float normalized = (db + 80) / 80;
                normalized = std::clamp(normalized, 0.0f, 1.0f);

                sf::Color color;
                if (ch == 0) {
                    color = sf::Color(255 * normalized, 0, 0);  // Левый — красный
                } else {
                    color = sf::Color(0, 0, 255 * normalized);  // Правый — синий
                }

                sf::RectangleShape pixel(sf::Vector2f(1, 1));
                pixel.setPosition(sf::Vector2f(x, height - y - 1));
                pixel.setFillColor(color);
                window.draw(pixel);
            }
        }
    }

    window.display();
}
