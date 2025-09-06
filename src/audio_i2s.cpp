#include "audio_source.h"

#if defined(PLATFORM_ESP32)
extern "C" {
#include "freertos/FreeRTOS.h"
#include "driver/i2s_std.h"
}
#include <cstdint>

#ifndef I2S_BCLK_PIN
#define I2S_BCLK_PIN 26
#endif
#ifndef I2S_WS_PIN
#define I2S_WS_PIN 25
#endif
#ifndef I2S_DATA_IN_PIN
#define I2S_DATA_IN_PIN 34
#endif

class Esp32I2SSource : public IAudioSource {
public:
    Esp32I2SSource(int sample_rate, int channels)
    : sample_rate_(sample_rate), channels_(channels) {
        // Crea canale RX standard
        i2s_chan_config_t chan_cfg = {};
        chan_cfg.id = I2S_NUM_0;
        chan_cfg.role = I2S_ROLE_MASTER;
        ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &rx_, nullptr));

        // Config clock
        i2s_std_clk_config_t clk{};
        clk.sample_rate_hz = sample_rate_;
        clk.clk_src = I2S_CLK_SRC_DEFAULT;
        clk.mclk_multiple = I2S_MCLK_MULTIPLE_256;

        // Config “slot” (formato dati I2S Philips)
        i2s_std_slot_config_t slot{};
        slot.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
        slot.slot_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
        slot.slot_mode = (channels_ == 2) ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO;
        slot.slot_mask = (channels_ == 2) ? I2S_STD_SLOT_BOTH : I2S_STD_SLOT_LEFT;
        slot.ws_width = I2S_DATA_BIT_WIDTH_16BIT;
        slot.ws_pol = false;
        slot.bit_shift = true;
        slot.left_align = true;
        slot.big_endian = false;
        slot.msb_right = false;

        // GPIO
        i2s_std_gpio_config_t gpio{};
        gpio.mclk = I2S_GPIO_UNUSED;
        gpio.bclk = (gpio_num_t)I2S_BCLK_PIN;
        gpio.ws   = (gpio_num_t)I2S_WS_PIN;
        gpio.dout = I2S_GPIO_UNUSED;
        gpio.din  = (gpio_num_t)I2S_DATA_IN_PIN;

        // Config aggregata
        i2s_std_config_t stdcfg{};
        stdcfg.clk_cfg  = clk;
        stdcfg.slot_cfg = slot;
        stdcfg.gpio_cfg = gpio;

        ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_, &stdcfg));
        ESP_ERROR_CHECK(i2s_channel_enable(rx_));
    }

    ~Esp32I2SSource() override {
        if (rx_) {
            i2s_channel_disable(rx_);
            i2s_del_channel(rx_);
        }
    }

    size_t read(int16_t* dst, size_t max_samples) override {
        size_t bytes_read = 0;
        // portMAX_DELAY è in FreeRTOS.h
        esp_err_t err = i2s_channel_read(rx_, dst, max_samples * sizeof(int16_t),
                                         &bytes_read, portMAX_DELAY);
        if (err != ESP_OK) return 0;
        return bytes_read / sizeof(int16_t);
    }

private:
    int sample_rate_;
    int channels_;
    i2s_channel_handle_t rx_{};
};

IAudioSource* makeEsp32I2SSource(int sample_rate, int channels) {
    return new Esp32I2SSource(sample_rate, channels);
}
#endif
