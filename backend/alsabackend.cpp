#include "alsabackend.h"

uint16_t AlsaBackend::read_u16_le(const uint8_t* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t AlsaBackend::read_u32_le(const uint8_t* p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

void AlsaBackend::playWAV(const char* file) {
    FILE* fp = fopen(file, "rb");

    if (!fp) {
        printf("Error opening WAV file \n");
        return;
    }

    uint8_t header[12];

    if (fread(header, 1, sizeof(header), fp) != sizeof(header) || memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        printf("Invalid WAV file.\n");
        fclose(fp);
        return;
    }

    unsigned int channels = 0;
    unsigned int sample_rate = 0;
    unsigned int bits_per_sample = 0;
    unsigned int audio_format = 0;

    uint32_t data_size = 0;
    long data_position = -1;

    while (true) {

        uint8_t chunk[8];

        if (fread(chunk, 1, sizeof(chunk), fp)
                != sizeof(chunk))
            break;

        uint32_t chunk_size = read_u32_le(chunk + 4);

        long chunk_start = ftell(fp);

        if (chunk_start < 0)
            break;

        if (memcmp(chunk, "fmt ", 4) == 0) {

            uint8_t format[16];

            if (chunk_size < sizeof(format) ||
                fread(format, 1, sizeof(format), fp)
                    != sizeof(format)) {

                printf("Invalid fmt chunk.\n");
                fclose(fp);
                return;
            }

            audio_format = read_u16_le(format);
            channels = read_u16_le(format + 2);
            sample_rate = read_u32_le(format + 4);
            bits_per_sample = read_u16_le(format + 14);

        } else if (memcmp(chunk, "data", 4) == 0) {

            data_position = chunk_start;
            data_size = chunk_size;
        }

        if (fseek(
                fp,
                chunk_start + static_cast<long>(chunk_size)
                    + (chunk_size & 1u),
                SEEK_SET) != 0) {
            break;
        }

        if (audio_format != 0 && data_position >= 0)
            break;
    }

    if (audio_format != 1 ||
        (channels != 1 && channels != 2) ||
        bits_per_sample != 16 ||
        sample_rate == 0 ||
        data_position < 0) {

        printf("Unsupported WAV format. Use PCM 16-bit, "
                "mono or stereo.\n");

        fclose(fp);
        return;
    }

    if (fseek(fp, data_position, SEEK_SET) != 0) {
        printf("Error seeking WAV file");
        fclose(fp);
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
        fclose(fp);
        return;
    }

    rc = snd_pcm_set_params(
        pcm,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        channels,
        sample_rate,
        1,
        500000
    );

    if (rc < 0) {
        printf("ALSA configuration error: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        fclose(fp);
        return;
    }

    uint8_t buffer[BUFFER_SIZE];

    uint32_t remaining = data_size;

    size_t bytes_per_frame = channels * 2;

    while (remaining >= bytes_per_frame) {

        size_t requested =
            remaining < sizeof(buffer)
                ? remaining
                : sizeof(buffer);

        requested -= requested % bytes_per_frame;

        size_t bytes_read = fread(buffer, 1, requested, fp);

        if (bytes_read == 0) break;

        bytes_read -= bytes_read % bytes_per_frame;

        snd_pcm_uframes_t frames = bytes_read / bytes_per_frame;

        uint8_t* data = buffer;

        while (frames > 0) {

            snd_pcm_sframes_t written = snd_pcm_writei(pcm, data, frames);

            if (written == -EPIPE) {
                snd_pcm_prepare(pcm);
                continue;
            }

            if (written < 0) {
                rc = snd_pcm_recover(pcm, static_cast<int>(written), 1);
                if (rc < 0) {
                    printf("Playback error: %s\n", snd_strerror(rc));
                    goto audio_cleanup;
                }
                continue;
            }

            if (written == 0) continue;

            size_t bytes_written = static_cast<size_t>(written) * bytes_per_frame;
            data += bytes_written;
            frames -= static_cast<snd_pcm_uframes_t>(written);
        }

        remaining -= static_cast<uint32_t>(bytes_read);

        if (bytes_read < requested)
            break;
    }

    snd_pcm_drain(pcm);

audio_cleanup:
    snd_pcm_close(pcm);
    fclose(fp);
}

void AlsaBackend::thread_audio(std::string file)
{
    playWAV(file.c_str());

    std::lock_guard<std::mutex> lock(audio_mutex);
    is_playing_audio = false;
}

void AlsaBackend::play(const char* file) {
    {
        std::lock_guard<std::mutex> lock(audio_mutex);

        if (is_playing_audio)
            return;

        is_playing_audio = true;
    }

    try {
        std::thread([this, file]() {thread_audio(file);}).detach();
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(audio_mutex);
        is_playing_audio = false;

        printf("Could not create audio thread.\n");
    }
}