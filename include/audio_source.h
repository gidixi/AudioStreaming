#pragma once
#include <cstdint>
#include <cstddef>

class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual size_t read(int16_t* dst, size_t max_samples) = 0;
};

IAudioSource* makePortAudioSource(int sample_rate, int channels);
IAudioSource* makeEsp32I2SSource(int sample_rate, int channels);
