#include "myfl_types.hpp"
#include "myfl_codec.hpp"
#include "wav_io.hpp"
#include "spectrogram.hpp"

#include <iostream>

int main() {
    initFFTW();

    // Загрузка аудио
    AudioData originalAudio = loadAudio(
        "/Users/maksimskorikov/Documents/2 курс/NIRS/A-Ha-Take_On_Me.wav"
    );
    if (originalAudio.samples.empty()) {
        std::cerr << "Failed to load input WAV" << std::endl;
        cleanupFFTW();
        return 1;
    }

    // Кодирование → декодирование → сохранение
    saveAsMYFL(originalAudio.samples, "output.MYFL",
               originalAudio.info.samplerate, originalAudio.info.channels);

    auto decoded = decodeMYFL("output.MYFL");
    saveAsWav(decoded, "decoded.wav",
              originalAudio.info.samplerate, originalAudio.info.channels,
              originalAudio.info.format & SF_FORMAT_SUBMASK);

    // Загрузка декодированного
    AudioData decodedAudio = loadAudio("decoded.wav");
    if (decodedAudio.samples.empty()) {
        std::cerr << "Failed to load decoded WAV" << std::endl;
        cleanupFFTW();
        return 1;
    }

    std::cout << "Original channels: " << originalAudio.info.channels << std::endl;
    std::cout << "Decoded channels: " << decodedAudio.info.channels << std::endl;
    std::cout << "Original samples: " << originalAudio.samples.size() << std::endl;
    std::cout << "Decoded samples: " << decodedAudio.samples.size() << std::endl;

    // Создание окон
    sf::RenderWindow windowOriginal(sf::VideoMode({800, 400}), "Original WAV");
    sf::RenderWindow windowDecoded(sf::VideoMode({800, 400}), "Decoded WAV");

    // Основной цикл
    while (windowOriginal.isOpen() && windowDecoded.isOpen()) {
        while (auto event = windowOriginal.pollEvent()) {
            if (event->is<sf::Event::Closed>()) windowOriginal.close();
        }
        while (auto event = windowDecoded.pollEvent()) {
            if (event->is<sf::Event::Closed>()) windowDecoded.close();
        }

        drawSpectrogram(windowOriginal, originalAudio.samples,
                        originalAudio.info.samplerate,
                        originalAudio.info.channels,
                        "Original WAV");

        drawSpectrogram(windowDecoded, decodedAudio.samples,
                        decodedAudio.info.samplerate,
                        decodedAudio.info.channels,
                        "Decoded WAV");
    }

    cleanupFFTW();
    return 0;
}
