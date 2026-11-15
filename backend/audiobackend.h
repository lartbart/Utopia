#pragma once

class AudioBackend {
    public:
        AudioBackend() = default;
        virtual ~AudioBackend() = default;
        virtual void play(const char* file) = 0;
};