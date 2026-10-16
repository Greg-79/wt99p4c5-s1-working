#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "ksdiy_p4c5_audio.h"

#define WIFI_SSID "ASUS123"
#define WIFI_PASS "kolokoL123"
#define WIFI_RETRY 5

static const char *TAG = "MAIN";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static int s_retry = 0;

extern esp_codec_dev_handle_t input_dev_;

static void event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry < WIFI_RETRY) {
            esp_wifi_connect();
            s_retry++;
            ESP_LOGI(TAG, "retry to connect");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to %s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "failed to connect to %s", WIFI_SSID);
    }
}

static void mic_task(void *args)
{
    const int BUF = 512;
    int16_t *buf = malloc(BUF * sizeof(int16_t));
    while (1) {
        if (esp_codec_dev_read(input_dev_, buf, BUF * sizeof(int16_t)) == ESP_OK) {
            int32_t peak = 0;
            int64_t sum = 0;
            for (int i = 0; i < BUF; i++) {
                int32_t v = buf[i] * 32;
                if (v > 32767) v = 32767;
                if (v < -32768) v = -32768;
                if (v < 0) v = -v;
                if (v > peak) peak = v;
                sum += (int64_t)v * v;
            }
            ESP_LOGI("MIC", "Peak: %ld, RMS: %.1f", (long)peak, sqrtf((float)sum / BUF));
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    // Инициализация аудио через компонент ksdiy_p4c5_audio
    // NULL и I2C_NUM_1 — для платы без дисплея, компонент сам создаст шину[citation:2][citation:4]
    Codec_I2S_init(NULL, I2C_NUM_1);

    wifi_init_sta();

    xTaskCreate(mic_task, "mic", 4096, NULL, 5, NULL);
}