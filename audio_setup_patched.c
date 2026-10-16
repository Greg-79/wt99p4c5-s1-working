/*
 * SPDX-FileCopyrightText: 2024-2026 Kevincoooool
 * SPDX-License-Identifier: Apache-2.0
 *
 * Адаптировано под WT99P4C5-S1: только ES8311 (duplex), без ES7210.
 */
#include "audio_setup.h"

#include <stdlib.h>
#include <string.h>

#include "driver/i2c_types.h"
#include "esp_check.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define EXAMPLE_BUFF_SIZE 2048

static const char *TAG = "AUDIO";

/* Заполнение fs для входа: моно, ES8311 ADC */
static void audio_fill_input_fs(esp_codec_dev_sample_info_t *fs)
{
    fs->bits_per_sample = 16;
    fs->channel = 1;
    fs->channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0);
    fs->sample_rate = 16000;
    fs->mclk_multiple = 0;
}

static i2c_port_t s_codec_i2c_port = I2C_NUM_1;

i2s_chan_handle_t tx_handle_ = NULL;
i2s_chan_handle_t rx_handle_ = NULL;
static const audio_codec_data_if_t *s_data_if = NULL;
static const audio_codec_ctrl_if_t *s_out_ctrl_if = NULL;
static const audio_codec_if_t *s_out_codec_if = NULL;
static const audio_codec_gpio_if_t *s_gpio_if = NULL;
esp_codec_dev_handle_t output_dev_ = NULL;
esp_codec_dev_handle_t input_dev_ = NULL;

esp_err_t audio_bus_init(i2c_master_bus_handle_t *bus_handle_out,
                           i2c_master_bus_handle_t external_bus,
                           i2c_port_t codec_i2c_port)
{
    ESP_RETURN_ON_FALSE(bus_handle_out, ESP_ERR_INVALID_ARG, TAG, "bus_handle_out is NULL");
    s_codec_i2c_port = codec_i2c_port;

    if (external_bus != NULL) {
        *bus_handle_out = external_bus;
        ESP_LOGI(TAG, "Using application-provided I2C bus (port %d)", (int)codec_i2c_port);
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_7,
        .scl_io_num = GPIO_NUM_8,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        },
    };
    esp_err_t err = i2c_new_master_bus(&i2c_bus_cfg, bus_handle_out);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Created dedicated I2C bus on GPIO7/8");
    }
    return err;
}

void audio_probe_i2c_devices(i2c_master_bus_handle_t bus_handle)
{
    if (!bus_handle) {
        return;
    }

    ESP_LOGI(TAG, "Scanning I2C bus for devices...");
    uint8_t found_count = 0;
    for (uint8_t i = 0x01; i < 0x7F; i++) {
        esp_err_t ret = i2c_master_probe(bus_handle, i, -1);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found I2C device at address: 0x%02X", i);
            found_count++;
        }
    }

    if (found_count == 0) {
        ESP_LOGW(TAG, "No I2C devices found on the bus");
    } else {
        ESP_LOGI(TAG, "Total %u I2C device(s) found", found_count);
    }
}

esp_err_t audio_create_duplex_channels(gpio_num_t mclk,
                                       gpio_num_t bclk,
                                       gpio_num_t ws,
                                       gpio_num_t dout,
                                       gpio_num_t din)
{
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 4,
        .dma_frame_num = 256,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_), TAG, "Create channel failed");

    /* Общий std-конфиг: и TX, и RX работают в std mode */
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = 16000,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = mclk,
            .bclk = bclk,
            .ws = ws,
            .dout = dout,
            .din = din,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_handle_, &std_cfg), TAG, "Init std TX failed");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx_handle_, &std_cfg), TAG, "Init std RX failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle_), TAG, "Enable TX failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx_handle_), TAG, "Enable RX failed");
    return ESP_OK;
}

esp_err_t audio_create_playback_tx_only(gpio_num_t mclk,
                                        gpio_num_t bclk,
                                        gpio_num_t ws,
                                        gpio_num_t dout,
                                        uint32_t sample_rate_hz)
{
    if (tx_handle_ != NULL) {
        i2s_channel_disable(tx_handle_);
        i2s_del_channel(tx_handle_);
        tx_handle_ = NULL;
    }
    if (rx_handle_ != NULL) {
        i2s_channel_disable(rx_handle_);
        i2s_del_channel(rx_handle_);
        rx_handle_ = NULL;
    }

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 240,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx_handle_, NULL), TAG, "Create TX channel failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = sample_rate_hz,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = mclk,
            .bclk = bclk,
            .ws = ws,
            .dout = dout,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_handle_, &std_cfg), TAG, "Init std TX failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle_), TAG, "Enable TX failed");
    return ESP_OK;
}

/* Создание ES8311 один раз, используется и для OUT, и для IN */
static esp_err_t audio_es8311_create(i2c_master_bus_handle_t bus_handle)
{
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle_,
        .tx_handle = tx_handle_,
    };
    s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    ESP_RETURN_ON_FALSE(s_data_if, ESP_ERR_NO_MEM, TAG, "data_if create failed");

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = s_codec_i2c_port,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = bus_handle,
    };
    s_out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    ESP_RETURN_ON_FALSE(s_out_ctrl_if, ESP_ERR_NO_MEM, TAG, "out_ctrl_if create failed");

    s_gpio_if = audio_codec_new_gpio();
    ESP_RETURN_ON_FALSE(s_gpio_if, ESP_ERR_NO_MEM, TAG, "gpio_if create failed");

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = s_out_ctrl_if;
    es8311_cfg.gpio_if = s_gpio_if;
    /* Duplex: DAC + ADC одновременно */
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH;
    es8311_cfg.pa_pin = GPIO_NUM_NC;
    es8311_cfg.use_mclk = true;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    s_out_codec_if = es8311_codec_new(&es8311_cfg);
    ESP_RETURN_ON_FALSE(s_out_codec_if, ESP_ERR_NO_MEM, TAG, "out_codec_if create failed");
    return ESP_OK;
}

esp_err_t audio_codec_init_playback_only(i2c_master_bus_handle_t bus_handle,
                                       const esp_codec_dev_sample_info_t *fs_out)
{
    ESP_RETURN_ON_FALSE(bus_handle && fs_out, ESP_ERR_INVALID_ARG, TAG, "invalid arg");
    ESP_RETURN_ON_FALSE(tx_handle_, ESP_ERR_INVALID_STATE, TAG, "TX channel not created");

    if (output_dev_) {
        esp_codec_dev_close(output_dev_);
        esp_codec_dev_delete(output_dev_);
        output_dev_ = NULL;
    }
    input_dev_ = NULL;

    ESP_RETURN_ON_ERROR(audio_es8311_create(bus_handle), TAG, "es8311 create failed");

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = s_out_codec_if,
        .data_if = s_data_if,
    };
    output_dev_ = esp_codec_dev_new(&dev_cfg);
    ESP_RETURN_ON_FALSE(output_dev_, ESP_ERR_NO_MEM, TAG, "output_dev create failed");

    ESP_RETURN_ON_ERROR(esp_codec_dev_open(output_dev_, (esp_codec_dev_sample_info_t *)fs_out), TAG, "open output failed");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(output_dev_, 90), TAG, "set out vol failed");
    return ESP_OK;
}

esp_err_t audio_output_reconfigure(const esp_codec_dev_sample_info_t *fs)
{
    ESP_RETURN_ON_FALSE(fs && output_dev_ && tx_handle_, ESP_ERR_INVALID_STATE, TAG, "audio not ready");

    ESP_RETURN_ON_ERROR(esp_codec_dev_close(output_dev_), TAG, "close output failed");
    esp_codec_dev_sample_info_t fs_mut = *fs;
    ESP_RETURN_ON_ERROR(esp_codec_dev_open(output_dev_, &fs_mut), TAG, "reopen ES8311 failed");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(output_dev_, 90), TAG, "set out vol failed");
    ESP_LOGI(TAG, "Output %lu Hz %u ch", (unsigned long)fs->sample_rate, (unsigned)fs->channel);
    return ESP_OK;
}

esp_err_t audio_input_reconfigure(const esp_codec_dev_sample_info_t *fs)
{
    ESP_RETURN_ON_FALSE(fs && input_dev_ && rx_handle_, ESP_ERR_INVALID_STATE, TAG, "input not ready");

    ESP_RETURN_ON_ERROR(esp_codec_dev_close(input_dev_), TAG, "close input failed");
    esp_codec_dev_sample_info_t fs_mut = *fs;
    if (fs_mut.channel_mask == 0) {
        audio_fill_input_fs(&fs_mut);
    }
    ESP_RETURN_ON_ERROR(esp_codec_dev_open(input_dev_, &fs_mut), TAG, "reopen ES8311 input failed");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_in_gain(input_dev_, 42.0), TAG, "set in gain failed");
    ESP_LOGI(TAG, "Input %lu Hz %u ch mask 0x%x", (unsigned long)fs_mut.sample_rate,
             (unsigned)fs_mut.channel, (unsigned)fs_mut.channel_mask);
    return ESP_OK;
}

esp_err_t audio_input_set_mono_record(void)
{
    esp_codec_dev_sample_info_t fs;
    audio_fill_input_fs(&fs);
    return audio_input_reconfigure(&fs);
}

esp_err_t audio_codec_set_fs(const esp_codec_dev_sample_info_t *fs_out,
                             const esp_codec_dev_sample_info_t *fs_in)
{
    ESP_RETURN_ON_FALSE(fs_out, ESP_ERR_INVALID_ARG, TAG, "fs_out is NULL");
    const esp_codec_dev_sample_info_t *in_fs = fs_in ? fs_in : fs_out;

    esp_err_t ret = ESP_OK;
    if (output_dev_) {
        ret = esp_codec_dev_close(output_dev_);
    }
    if (input_dev_) {
        ret |= esp_codec_dev_close(input_dev_);
    }
    ESP_RETURN_ON_ERROR(ret, TAG, "close codec failed");

    esp_codec_dev_sample_info_t fs_out_mut = *fs_out;
    esp_codec_dev_sample_info_t fs_in_mut = *in_fs;

    if (output_dev_) {
        ESP_RETURN_ON_ERROR(esp_codec_dev_open(output_dev_, &fs_out_mut), TAG, "open ES8311 output failed");
        ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(output_dev_, 90), TAG, "set out vol failed");
    }
    if (input_dev_) {
        if (fs_in_mut.channel_mask == 0) {
            audio_fill_input_fs(&fs_in_mut);
        }
        ESP_RETURN_ON_ERROR(esp_codec_dev_open(input_dev_, &fs_in_mut), TAG, "open ES8311 input failed");
        ESP_RETURN_ON_ERROR(esp_codec_dev_set_in_gain(input_dev_, 42.0), TAG, "set in gain failed");
    }
    ESP_LOGI(TAG, "Codec out(ES8311) %lu Hz %u ch, in(ES8311) %lu Hz %u ch",
             (unsigned long)fs_out->sample_rate, (unsigned)fs_out->channel,
             (unsigned long)in_fs->sample_rate, (unsigned)in_fs->channel);
    return ESP_OK;
}

esp_err_t audio_codec_init(i2c_master_bus_handle_t bus_handle)
{
    ESP_RETURN_ON_FALSE(bus_handle, ESP_ERR_INVALID_ARG, TAG, "bus_handle is NULL");

    ESP_RETURN_ON_ERROR(audio_es8311_create(bus_handle), TAG, "es8311 create failed");

    /* OUT dev */
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = s_out_codec_if,
        .data_if = s_data_if,
    };
    output_dev_ = esp_codec_dev_new(&dev_cfg);
    ESP_RETURN_ON_FALSE(output_dev_, ESP_ERR_NO_MEM, TAG, "output_dev create failed");

    /* IN dev — тот же ES8311, но как вход */
    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = s_out_codec_if;
    dev_cfg.data_if = s_data_if;
    input_dev_ = esp_codec_dev_new(&dev_cfg);
    ESP_RETURN_ON_FALSE(input_dev_, ESP_ERR_NO_MEM, TAG, "input_dev create failed");

    /* Сначала открываем выход */
    esp_codec_dev_sample_info_t fs_out = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = 16000,
        .mclk_multiple = 0,
    };
    ESP_RETURN_ON_ERROR(esp_codec_dev_open(output_dev_, &fs_out), TAG, "open output failed");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(output_dev_, 90), TAG, "set out vol failed");

    /* Потом вход */
    esp_codec_dev_sample_info_t fs_in;
    audio_fill_input_fs(&fs_in);
    ESP_RETURN_ON_ERROR(esp_codec_dev_open(input_dev_, &fs_in), TAG, "open input failed");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_in_gain(input_dev_, 42.0), TAG, "set in gain failed");

    ESP_LOGI(TAG, "ES8311 duplex ready (DAC+ADC)");
    return ESP_OK;
}

static void audio_echo_task(void *args)
{
    int *mic_data = malloc(EXAMPLE_BUFF_SIZE);
    if (!mic_data) {
        ESP_LOGE(TAG, "[echo] No memory for read data buffer");
        abort();
    }

    size_t bytes_read = 0;
    size_t bytes_write = 0;

    while (1) {
        memset(mic_data, 0, EXAMPLE_BUFF_SIZE);
        ESP_ERROR_CHECK(i2s_channel_read(rx_handle_, mic_data, EXAMPLE_BUFF_SIZE, &bytes_read, 100));
        ESP_ERROR_CHECK(i2s_channel_write(tx_handle_, mic_data, EXAMPLE_BUFF_SIZE, &bytes_write, 100));
    }
}

esp_err_t audio_start_echo_task(void)
{
    BaseType_t ret = xTaskCreate(audio_echo_task, "i2s_echo", 8 * 1024, NULL, 5, NULL);
    return ret == pdPASS ? ESP_OK : ESP_FAIL;
}