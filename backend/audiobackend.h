#pragma once

#include "../core/sound.h"

class AudioBackend {
    public:
        AudioBackend() = default;
        virtual ~AudioBackend() = default;
        virtual void play(const AudioData& audio) = 0;
};