#pragma once

#include <cstdint>
#include <vector>
#include "string.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

enum AudioFormat {
    WAV,
    MP3,
    UNKNOWN
};

struct AudioData {
    std::vector<int16_t> samples;
    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    unsigned int bitsPerSample = 16;
    size_t totalFrames = 0;
};

class Sound {
    public:
        Sound() = default;
        ~Sound() = default;
        AudioData audio_data;
        void loadAudio(const char* file);
    private:
        AudioFormat getAudioFormat(const char* file);
        void loadWav(const char* file);
};