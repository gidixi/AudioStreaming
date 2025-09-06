// ================= src/audio_esp32.cpp (stub) =================
#if defined(PLATFORM_ESP32)
#include "driver/i2s.h"
class Esp32I2SSource : public IAudioSource {
public:
    Esp32I2SSource(int sr, int ch){ /* configure i2s, pins, format = S16 mono */ }
    size_t read(int16_t* dst, size_t max_samples) override {
        size_t bytes_read = 0; // i2s_read(...)
        return bytes_read / sizeof(int16_t);
    }
};
IAudioSource* makeEsp32I2SSource(int sr, int ch){ return new Esp32I2SSource(sr, ch);} 
#endif

