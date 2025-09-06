#include "opus_codec.h"

#include <opus/opus.h>
#include <stdexcept>
#include <cstdint>

class OpusEncImpl : public OpusEncoderWrapper {
public:
    OpusEncImpl(int sr, int ch, int br) {
        int err = 0;
        enc_ = opus_encoder_create(sr, ch, OPUS_APPLICATION_VOIP, &err);
        if (err != OPUS_OK) throw std::runtime_error("opus enc create");
        opus_encoder_ctl(enc_, OPUS_SET_BITRATE(br));
        opus_encoder_ctl(enc_, OPUS_SET_VBR(1));
        opus_encoder_ctl(enc_, OPUS_SET_COMPLEXITY(5));
    }
    ~OpusEncImpl() override { opus_encoder_destroy(enc_); }
    int encode(const int16_t* pcm, int frame_size, uint8_t* out, int max_out) override {
        return (int)opus_encode(enc_, pcm, frame_size, out, max_out);
    }
private:
    OpusEncoder* enc_{};
};

class OpusDecImpl : public OpusDecoderWrapper {
public:
    OpusDecImpl(int sr, int ch) {
        int err = 0;
        dec_ = opus_decoder_create(sr, ch, &err);
        if (err != OPUS_OK) throw std::runtime_error("opus dec create");
    }
    ~OpusDecImpl() override { opus_decoder_destroy(dec_); }
    int decode(const uint8_t* data, int len, int16_t* pcm, int max_samples) override {
        return opus_decode(dec_, data, len, pcm, max_samples, 0);
    }
private:
    OpusDecoder* dec_{};
};

OpusEncoderWrapper* makeOpusEncoder(int sr, int ch, int br) { return new OpusEncImpl(sr, ch, br); }
OpusDecoderWrapper* makeOpusDecoder(int sr, int ch)         { return new OpusDecImpl(sr, ch); }
