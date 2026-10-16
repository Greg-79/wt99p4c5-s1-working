WT99P4C5-S1 — рабочий пример
Рабочий пример для платы Wireless-Tag WT99P4C5-S1 (ESP32-P4 + ESP32-C5) с Wi-Fi через SDIO, аудиокодеком ES8311 и мониторингом уровня микрофона.

Аппаратная часть
Плата: Wireless-Tag WT99P4C5-S1

Основной MCU: ESP32-P4 (revision v1.0)

Сопроцессор: ESP32-C5 (revision v1.0)

Аудиокодек: ES8311 (I2C адрес 0x18)

Что работает
✅ Wi-Fi через ESP32-C5 по SDIO (ESP-Hosted 3.0.9)

✅ Аудио duplex: динамик (DAC) + микрофон (ADC)

✅ Мониторинг уровня микрофона (Peak/RMS в логах)

Распиновка
SDIO (P4 ↔ C5)
Сигнал	GPIO
CLK	18
CMD	19
D0	14
D1	15
D2	16
D3	17
RESET	54
I2C (ES8311)
Сигнал	GPIO
SDA	7
SCL	8
I2S
Сигнал	GPIO
MCLK	13
BCLK	12
WS	10
DOUT	9
DIN	11
Известные проблемы
1. Конфликт с ES7210
Стандартный компонент ksdiy_p4c5_audio пытается инициализировать ES7210, которого нет на этой плате. Микрофон на WT99P4C5-S1 подключён напрямую к ES8311.

Решение: После первой сборки (idf.py build) замени файл:

text
managed_components/kevincoooool__ksdiy_p4c5_audio/audio_setup.c
на файл audio_setup_patched.c из корня проекта, затем пересобери:

bash
idf.py build
idf.py -p COM_PORT flash
2. SD-карта не тестировалась
Существует известный конфликт между ESP-Hosted (использует SDIO slot 1) и SD-картой (slot 0) в ESP-IDF. SD-карта в этом примере не используется.

Сборка
bash
idf.py set-target esp32p4
idf.py build
Важно: После первой сборки замени audio_setup.c на audio_setup_patched.c (см. выше), затем пересобери.

Прошивка
bash
idf.py -p COM_PORT flash monitor
Ожидаемый лог
text
I (xxx) AUDIO: Created dedicated I2C bus on GPIO7/8
I (xxx) AUDIO: Found I2C device at address: 0x18
I (xxx) ES8311: Work in Slave mode
I (xxx) AUDIO: ES8311 duplex ready (DAC+ADC)
I (xxx) eh_init_evt: slave chip id: 0x17 (esp32c5)
I (xxx) eh_init_evt: esp-hosted fw versions: host=3.0.9 coprocessor=3.0.9 (match)
I (xxx) MAIN: got ip:192.168.x.x
I (xxx) MAIN: connected to ASUS123
I (xxx) MIC: Peak: 13472, RMS: 5423.6
Примечания
Прошивка C5: ESP-Hosted 3.0.9 (собрана из esp-hosted-mcu)

ESP-IDF: v5.5.2

Для сборки прошивки C5 используйте пример examples/wifi/sta/cp из репозитория esp-hosted-mcu

Ссылки
ESP-Hosted-MCU

ESP-IDF

espressif/esp_codec_dev