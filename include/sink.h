#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

class IPcmSink {
public:
    virtual ~IPcmSink() = default;
    virtual void write(const int16_t* pcm, size_t samples) = 0;
};

IPcmSink* makeFilePcmSink(const std::string& path);

class RingBufferSink : public IPcmSink {
public:
    explicit RingBufferSink(size_t capacity_samples) : buf_(capacity_samples) {}
    void write(const int16_t* pcm, size_t samples) override {
        for (size_t i = 0; i < samples; ++i) {
            buf_[w_] = pcm[i];
            w_ = (w_ + 1) % buf_.size();
            if (size_ < buf_.size()) ++size_;
            else r_ = (r_ + 1) % buf_.size();
        }
    }
    size_t read(int16_t* out, size_t max_samples) {
        size_t n = std::min(max_samples, size_);
        for (size_t i = 0; i < n; ++i) out[i] = buf_[(r_ + i) % buf_.size()];
        r_ = (r_ + n) % buf_.size();
        size_ -= n;
        return n;
    }
    size_t size() const { return size_; }
private:
    std::vector<int16_t> buf_;
    size_t r_ = 0, w_ = 0, size_ = 0;
};
