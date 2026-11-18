#include "alsabackend.h"

void AlsaBackend::playAudio() {

    if (this->currentAudio.samples.empty() ||
        this->currentAudio.channels == 0 ||
        this->currentAudio.sampleRate == 0) {
        return;
    }

    snd_pcm_t* pcm = nullptr;

    const char* audio_device = "plughw:0,0";

    int rc = snd_pcm_open(
        &pcm,
        audio_device,
        SND_PCM_STREAM_PLAYBACK,
        0
    );

    if (rc < 0) {
        printf("ALSA error: %s\n", snd_strerror(rc));
        return;
    }

    rc = snd_pcm_set_params(
        pcm,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        this->currentAudio.channels,
        this->currentAudio.sampleRate,
        1,
        500000
    );

    if (rc < 0) {
        printf("ALSA configuration error: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        return;
    }

    constexpr size_t BUFFER_FRAMES = 4096;

    size_t offset = 0;

    while (offset < this->currentAudio.totalFrames) {

        snd_pcm_uframes_t frames = std::min(
            BUFFER_FRAMES,
            this->currentAudio.totalFrames - offset
        );

        const int16_t* data = this->currentAudio.samples.data() + offset * this->currentAudio.channels;

        snd_pcm_sframes_t written = snd_pcm_writei(
            pcm,
            data,
            frames
        );

        if (written == -EPIPE) {
            snd_pcm_prepare(pcm);
            continue;
        }

        if (written < 0) {

            rc = snd_pcm_recover(
                pcm,
                static_cast<int>(written),
                1
            );

            if (rc < 0) {
                printf("Playback error: %s\n", snd_strerror(rc));
                break;
            }

            continue;
        }

        if (written == 0)
            continue;

        offset += static_cast<size_t>(written);
    }

    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
}

void AlsaBackend::thread_audio()
{
    playAudio();

    std::lock_guard<std::mutex> lock(audio_mutex);
    is_playing_audio = false;
}

void AlsaBackend::play(const AudioData& audio) {
    this->currentAudio = audio;
    {
        std::lock_guard<std::mutex> lock(audio_mutex);

        if (is_playing_audio)
            return;

        is_playing_audio = true;
    }

    try {
        std::thread([this]() {thread_audio();}).detach();
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(audio_mutex);
        is_playing_audio = false;

        printf("Could not create audio thread.\n");
    }
}