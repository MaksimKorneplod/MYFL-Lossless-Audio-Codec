#include "wav_io.hpp"
#include <sndfile.h>
#include <iostream>
#include <algorithm>

AudioData loadAudio(const char* filename) {
    AudioData data;
    SNDFILE* file = sf_open(filename, SFM_READ, &data.info);
    if (!file) {
        std::cerr << "Error opening audio file: " << filename << " - " << sf_strerror(nullptr) << std::endl;
        return data;
    }

    data.samples.resize(data.info.frames * data.info.channels);
    sf_readf_float(file, data.samples.data(), data.info.frames);
    sf_close(file);

    return data;
}

void saveAsWav(const std::vector<float>& samples, const std::string& filename,
               int sampleRate, int channels, int format) {
    SF_INFO info{};
    info.samplerate = sampleRate;
    info.channels = channels;
    info.format = SF_FORMAT_WAV | format;

    SNDFILE* file = sf_open(filename.c_str(), SFM_WRITE, &info);
    if (!file) {
        std::cerr << "Error saving WAV: " << sf_strerror(nullptr) << std::endl;
        return;
    }

    int subtype = format & SF_FORMAT_SUBMASK;

    if (subtype == SF_FORMAT_PCM_16) {
        std::vector<int16_t> pcm(samples.size());
        for (size_t i = 0; i < samples.size(); ++i) {
            pcm[i] = static_cast<int16_t>(
                std::round(std::clamp(samples[i] * 32767.0f, -32768.0f, 32767.0f))
            );
        }
        sf_write_short(file, pcm.data(), pcm.size());
    } else if (subtype == SF_FORMAT_PCM_24 || subtype == SF_FORMAT_PCM_32) {
        std::vector<int32_t> pcm(samples.size());
        int scale = (subtype == SF_FORMAT_PCM_24) ? 8388607 : 2147483647;
        for (size_t i = 0; i < samples.size(); ++i) {
            pcm[i] = static_cast<int32_t>(
                std::round(std::clamp(samples[i], -1.0f, 1.0f) * scale)
            );
        }
        sf_write_int(file, pcm.data(), pcm.size());
    } else if (subtype == SF_FORMAT_FLOAT) {
        sf_write_float(file, samples.data(), samples.size());
    } else {
        std::cerr << "Unsupported format for writing WAV." << std::endl;
    }

    sf_close(file);
    std::cout << "Saved as WAV: " << filename << std::endl;
}
