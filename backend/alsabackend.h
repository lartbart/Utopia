#pragma once

#include "audiobackend.h"
#include <thread>
#include <mutex>
#include <string>
#include <alsa/asoundlib.h>
#include "stdio.h"

#define BUFFER_SIZE 4096

class AlsaBackend : public AudioBackend {
    public:
        AlsaBackend() = default;
        ~AlsaBackend() override = default;
        void play(const char* file) override;
    private:
        uint16_t read_u16_le(const uint8_t* p);
        uint32_t read_u32_le(const uint8_t* p);
        void playWAV(const char* file);
        void thread_audio(std::string file);
        std::mutex audio_mutex;
        bool is_playing_audio = false;
};