[English](#english) | [Русский](#русский)

---

# English

# WT99P4C5-S1 Working Example

ESP32-P4 + ESP32-C5 (SDIO) working example with Wi-Fi, ES8311 audio, and microphone level monitoring.

## Hardware

- **Board**: Wireless-Tag WT99P4C5-S1
- **Main MCU**: ESP32-P4 (rev v1.0)
- **Coprocessor**: ESP32-C5 (rev v1.0)
- **Audio Codec**: ES8311 (I2C address 0x18)

## What Works

- ✅ Wi-Fi via ESP32-C5 over SDIO (ESP-Hosted 3.0.9)
- ✅ Audio duplex: speaker (DAC) + microphone (ADC)
- ✅ Microphone level monitoring (Peak/RMS in logs)

## Pin Configuration

### SDIO (P4 ↔ C5)

| Signal | GPIO |
|--------|------|
| CLK    | 18   |
| CMD    | 19   |
| D0     | 14   |
| D1     | 15   |
| D2     | 16   |
| D3     | 17   |
| RESET  | 54   |

### I2C (ES8311)

| Signal | GPIO |
|--------|------|
| SDA    | 7    |
| SCL    | 8    |

### I2S

| Signal | GPIO |
|--------|------|
| MCLK   | 13   |
| BCLK   | 12   |
| WS     | 10   |
| DOUT   | 9    |
| DIN    | 11   |

## Known Issues

### 1. ES7210 conflict

The stock `ksdiy_p4c5_audio` component tries to init **ES7210**, which is **NOT** present on this board. The microphone on WT99P4C5-S1 is connected directly to ES8311.

**Fix:** After the first build (`idf.py build`), replace:

```
managed_components/kevincoooool__ksdiy_p4c5_audio/audio_setup.c
```

with `audio_setup_patched.c` from the project root, then rebuild:

```bash
idf.py build
idf.py -p COM_PORT flash
```

### 2. SD card not tested

There is a known conflict between ESP-Hosted (SDIO slot 1) and SD card (slot 0) in ESP-IDF. SD card is not used in this example.

## Build

```bash
idf.py set-target esp32p4
idf.py build
```

**Important:** After the first build, replace `audio_setup.c` with `audio_setup_patched.c` (see above), then rebuild.

## Flash

```bash
idf.py -p COM_PORT flash monitor
```

## Expected Log

```
I (xxx) AUDIO: Created dedicated I2C bus on GPIO7/8
I (xxx) AUDIO: Found I2C device at address: 0x18
I (xxx) ES8311: Work in Slave mode
I (xxx) AUDIO: ES8311 duplex ready (DAC+ADC)
I (xxx) eh_init_evt: slave chip id: 0x17 (esp32c5)
I (xxx) eh_init_evt: esp-hosted fw versions: host=3.0.9 coprocessor=3.0.9 (match)
I (xxx) MAIN: got ip:192.168.x.x
I (xxx) MAIN: connected to ASUS123
I (xxx) MIC: Peak: 13472, RMS: 5423.6
```
## C5 Firmware

The ESP32-C5 coprocessor must be flashed with ESP-Hosted firmware. A pre-built binary is included in the `c5_firmware/` directory.

See [`c5_firmware/README.md`](c5_firmware/README.md) for detailed flashing instructions.

Quick start:

```bash
esptool.py --chip esp32c5 --port COM_PORT_C5 --baud 460800 erase_flash
esptool.py --chip esp32c5 --port COM_PORT_C5 --baud 460800 write_flash 0x0 c5_firmware/eh_cp_wifi_sta.bin
```
## Notes

- C5 firmware: ESP-Hosted **3.0.9** (built from `esp-hosted-mcu`)
- ESP-IDF: **v5.5.2**
- To build C5 firmware, use `examples/wifi/sta/cp` from the `esp-hosted-mcu` repository

## Links

- [ESP-Hosted-MCU](https://github.com/espressif/esp-hosted-mcu)
- [ESP-IDF](https://github.com/espressif/esp-idf)
- [espressif/esp_codec_dev](https://components.espressif.com/components/espressif/esp_codec_dev)

---

# Русский

# WT99P4C5-S1 — рабочий пример

Рабочий пример для платы **Wireless-Tag WT99P4C5-S1** (ESP32-P4 + ESP32-C5) с Wi-Fi через SDIO, аудиокодеком ES8311 и мониторингом уровня микрофона.

## Аппаратная часть

- **Плата**: Wireless-Tag WT99P4C5-S1
- **Основной MCU**: ESP32-P4 (revision v1.0)
- **Сопроцессор**: ESP32-C5 (revision v1.0)
- **Аудиокодек**: ES8311 (I2C адрес 0x18)

## Что работает

- ✅ Wi-Fi через ESP32-C5 по SDIO (ESP-Hosted 3.0.9)
- ✅ Аудио duplex: динамик (DAC) + микрофон (ADC)
- ✅ Мониторинг уровня микрофона (Peak/RMS в логах)

## Распиновка

### SDIO (P4 ↔ C5)

| Сигнал | GPIO |
|--------|------|
| CLK    | 18   |
| CMD    | 19   |
| D0     | 14   |
| D1     | 15   |
| D2     | 16   |
| D3     | 17   |
| RESET  | 54   |

### I2C (ES8311)

| Сигнал | GPIO |
|--------|------|
| SDA    | 7    |
| SCL    | 8    |

### I2S

| Сигнал | GPIO |
|--------|------|
| MCLK   | 13   |
| BCLK   | 12   |
| WS     | 10   |
| DOUT   | 9    |
| DIN    | 11   |

## Известные проблемы

### 1. Конфликт с ES7210

Стандартный компонент `ksdiy_p4c5_audio` пытается инициализировать **ES7210**, которого **нет** на этой плате. Микрофон на WT99P4C5-S1 подключён напрямую к ES8311.

**Решение:** После первой сборки (`idf.py build`) замени файл:

```
managed_components/kevincoooool__ksdiy_p4c5_audio/audio_setup.c
```

на файл `audio_setup_patched.c` из корня проекта, затем пересобери:

```bash
idf.py build
idf.py -p COM_PORT flash
```

### 2. SD-карта не тестировалась

Существует известный конфликт между ESP-Hosted (использует SDIO slot 1) и SD-картой (slot 0) в ESP-IDF. SD-карта в этом примере не используется.

## Сборка

```bash
idf.py set-target esp32p4
idf.py build
```

**Важно:** После первой сборки замени `audio_setup.c` на `audio_setup_patched.c` (см. выше), затем пересобери.

## Прошивка

```bash
idf.py -p COM_PORT flash monitor
```

## Ожидаемый лог

```
I (xxx) AUDIO: Created dedicated I2C bus on GPIO7/8
I (xxx) AUDIO: Found I2C device at address: 0x18
I (xxx) ES8311: Work in Slave mode
I (xxx) AUDIO: ES8311 duplex ready (DAC+ADC)
I (xxx) eh_init_evt: slave chip id: 0x17 (esp32c5)
I (xxx) eh_init_evt: esp-hosted fw versions: host=3.0.9 coprocessor=3.0.9 (match)
I (xxx) MAIN: got ip:192.168.x.x
I (xxx) MAIN: connected to ASUS123
I (xxx) MIC: Peak: 13472, RMS: 5423.6
```
## Прошивка C5

Сопроцессор ESP32-C5 должен быть прошит прошивкой ESP-Hosted. Готовый бинарник включён в директорию `c5_firmware/`.

Подробная инструкция — в файле [`c5_firmware/README.md`](c5_firmware/README.md).

Быстрый старт:

```bash
esptool.py --chip esp32c5 --port COM_PORT_C5 --baud 460800 erase_flash
esptool.py --chip esp32c5 --port COM_PORT_C5 --baud 460800 write_flash 0x0 c5_firmware/eh_cp_wifi_sta.bin
```
## Примечания

- Прошивка C5: ESP-Hosted **3.0.9** (собрана из `esp-hosted-mcu`)
- ESP-IDF: **v5.5.2**
- Для сборки прошивки C5 используйте пример `examples/wifi/sta/cp` из репозитория `esp-hosted-mcu`

## Ссылки

- [ESP-Hosted-MCU](https://github.com/espressif/esp-hosted-mcu)
- [ESP-IDF](https://github.com/espressif/esp-idf)
- [espressif/esp_codec_dev](https://components.espressif.com/components/espressif/esp_codec_dev)