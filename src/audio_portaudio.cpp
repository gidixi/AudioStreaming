#include "audio_source.h"

#if defined(PLATFORM_DESKTOP)
#include <portaudio.h>

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

class PortAudioSource : public IAudioSource {
public:
    PortAudioSource(int sr, int ch) : sample_rate_(sr), channels_(ch) {
        Pa_Initialize();
        PaStreamParameters in{};
        in.channelCount = ch;
        in.device = Pa_GetDefaultInputDevice();
        in.sampleFormat = paInt16;
        if (in.device != paNoDevice) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(in.device);
            in.suggestedLatency = info ? info->defaultLowInputLatency : 0;
        } else {
            in.suggestedLatency = 0;
        }
        in.hostApiSpecificStreamInfo = nullptr;
        Pa_OpenStream(&stream_, &in, nullptr, sr,
                      paFramesPerBufferUnspecified, paNoFlag, &PortAudioSource::cb, this);
        Pa_StartStream(stream_);
    }
    ~PortAudioSource() {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
        Pa_Terminate();
    }

    size_t read(int16_t* dst, size_t max_samples) override {
        std::unique_lock<std::mutex> lk(m_);
        while (fifo_.size() < max_samples) cv_.wait(lk);
        size_t n = max_samples;
        for (size_t i = 0; i < n; ++i) dst[i] = fifo_[i];
        fifo_.erase(fifo_.begin(), fifo_.begin() + n);
        return n;
    }
private:
    static int cb(const void* input, void*, unsigned long frameCount,
                  const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* user) {
        auto* self = (PortAudioSource*)user;
        const int16_t* in = (const int16_t*)input;
        if (!in) return paContinue;
        size_t samples = frameCount * self->channels_;
        {
            std::lock_guard<std::mutex> lg(self->m_);
            self->fifo_.insert(self->fifo_.end(), in, in + samples);
        }
        self->cv_.notify_all();
        return paContinue;
    }

    int sample_rate_;
    int channels_;
    PaStream* stream_{};
    std::vector<int16_t> fifo_;
    std::mutex m_;
    std::condition_variable cv_;
};

IAudioSource* makePortAudioSource(int sr, int ch) { return new PortAudioSource(sr, ch); }
#endif
