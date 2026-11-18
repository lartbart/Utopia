#pragma once

#include "audiobackend.h"
#include <thread>
#include <mutex>
#include <alsa/asoundlib.h>
#include "stdio.h"


class AlsaBackend : public AudioBackend {
    public:
        AlsaBackend() = default;
        ~AlsaBackend() override = default;
        void play(const AudioData& audio) override;
    private:
        void playAudio();
        void thread_audio();
        std::mutex audio_mutex;
        bool is_playing_audio = false;
        AudioData currentAudio;
};