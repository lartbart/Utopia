#include "sound.h"

static uint16_t read_u16_le(const uint8_t* p) {
    return p[0] | (p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

AudioFormat Sound::getAudioFormat(const char* file) {

    if (!file) return AudioFormat::UNKNOWN;

    std::string filename(file);

    size_t pos = filename.find_last_of('.');

    if (pos == std::string::npos) return AudioFormat::UNKNOWN;

    std::string extension = filename.substr(pos);

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char c) {
            return std::tolower(c);
        }
    );

    if (extension == ".wav")
        return AudioFormat::WAV;

    if (extension == ".mp3")
        return AudioFormat::MP3;

    return AudioFormat::UNKNOWN;
}

void Sound::loadAudio(const char* file) {
    if (getAudioFormat(file) == AudioFormat::WAV) loadWav(file);
}

void Sound::loadWav(const char* file) {

    AudioData audio;

    FILE* fp = fopen(file, "rb");

    if (!fp)
        throw std::runtime_error("Error opening WAV file");

    try {

        uint8_t header[12];

        if (fread(header, 1, sizeof(header), fp) != sizeof(header) ||
            memcmp(header, "RIFF", 4) != 0 ||
            memcmp(header + 8, "WAVE", 4) != 0) {

            throw std::runtime_error("Invalid WAV file");
        }

        unsigned int channels = 0;
        unsigned int sample_rate = 0;
        unsigned int bits_per_sample = 0;
        unsigned int audio_format = 0;

        uint32_t data_size = 0;
        long data_position = -1;

        while (true) {

            uint8_t chunk[8];

            if (fread(chunk, 1, sizeof(chunk), fp) != sizeof(chunk))
                break;

            uint32_t chunk_size = read_u32_le(chunk + 4);

            long chunk_start = ftell(fp);

            if (chunk_start < 0)
                throw std::runtime_error("Invalid WAV chunk");

            if (memcmp(chunk, "fmt ", 4) == 0) {

                uint8_t format[16];

                if (chunk_size < sizeof(format) ||
                    fread(format, 1, sizeof(format), fp) != sizeof(format)) {

                    throw std::runtime_error("Invalid WAV fmt chunk");
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

                throw std::runtime_error("Error reading WAV chunks");
            }

            if (audio_format != 0 && data_position >= 0)
                break;
        }

        if (audio_format != 1 ||
            (channels != 1 && channels != 2) ||
            bits_per_sample != 16 ||
            sample_rate == 0 ||
            data_position < 0) {

            throw std::runtime_error("Unsupported WAV format");
        }

        size_t bytes_per_frame = channels * 2;

        if (data_size % bytes_per_frame != 0)
            throw std::runtime_error("Invalid WAV data size");

        if (fseek(fp, data_position, SEEK_SET) != 0)
            throw std::runtime_error("Error seeking WAV data");

        size_t total_samples = data_size / sizeof(int16_t);

        audio.samples.resize(total_samples);

        if (fread(
                audio.samples.data(),
                1,
                data_size,
                fp) != data_size) {

            throw std::runtime_error("Error reading WAV samples");
        }

        audio.channels = channels;
        audio.sampleRate = sample_rate;
        audio.bitsPerSample = bits_per_sample;
        audio.totalFrames = data_size / bytes_per_frame;

    } catch (...) {

        fclose(fp);
        throw;
    }

    fclose(fp);

    this->audio_data = audio;
}
