#pragma once
#include <cstdint>

class OpusEncoderWrapper {
public:
    virtual ~OpusEncoderWrapper() = default;
    virtual int encode(const int16_t* pcm, int frame_size, uint8_t* out, int max_out) = 0;
};

class OpusDecoderWrapper {
public:
    virtual ~OpusDecoderWrapper() = default;
    virtual int decode(const uint8_t* data, int len, int16_t* pcm, int max_samples) = 0;
};

OpusEncoderWrapper* makeOpusEncoder(int sample_rate, int channels, int bitrate_bps);
OpusDecoderWrapper* makeOpusDecoder(int sample_rate, int channels);
