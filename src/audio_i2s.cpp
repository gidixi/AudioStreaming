#include "audio_source.h"

#if defined(PLATFORM_ESP32)
extern "C" {
#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
}

class Esp32I2SSource : public IAudioSource {
public:
    Esp32I2SSource(int sample_rate, int channels)
        : sample_rate_(sample_rate), channels_(channels) {
        i2s_config_t config = {};
        config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
        config.sample_rate = sample_rate_;
        config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        config.channel_format = (channels_ == 2)
                                   ? I2S_CHANNEL_FMT_RIGHT_LEFT
                                   : I2S_CHANNEL_FMT_ONLY_LEFT;
        config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
        config.dma_buf_count = 4;
        config.dma_buf_len = 512;
        config.use_apll = false;
        config.tx_desc_auto_clear = false;
        config.fixed_mclk = 0;

        i2s_driver_install(I2S_NUM_0, &config, 0, nullptr);

        i2s_pin_config_t pin_cfg = {};
        pin_cfg.bck_io_num = I2S_PIN_NO_CHANGE;
        pin_cfg.ws_io_num = I2S_PIN_NO_CHANGE;
        pin_cfg.data_out_num = I2S_PIN_NO_CHANGE;
        pin_cfg.data_in_num = I2S_PIN_NO_CHANGE;
        i2s_set_pin(I2S_NUM_0, &pin_cfg);
        i2s_zero_dma_buffer(I2S_NUM_0);
    }

    ~Esp32I2SSource() override { i2s_driver_uninstall(I2S_NUM_0); }

    size_t read(int16_t *dst, size_t max_samples) override {
        size_t bytes_read = 0;
        i2s_read(I2S_NUM_0, dst, max_samples * sizeof(int16_t), &bytes_read,
                 portMAX_DELAY);
        return bytes_read / sizeof(int16_t);
    }

private:
    int sample_rate_;
    int channels_;
};

IAudioSource *makeEsp32I2SSource(int sample_rate, int channels) {
    return new Esp32I2SSource(sample_rate, channels);
}
#endif
