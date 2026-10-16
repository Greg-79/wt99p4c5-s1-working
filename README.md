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
managed_components/kevincoooool__ksdiy_p4c5_audio/audio_setup.c

text

with `audio_setup_patched.c` from the project root, then rebuild:

```bash
idf.py build
idf.py -p COM_PORT flash
2. SD card not tested
There is a known conflict between ESP-Hosted (SDIO slot 1) and SD card (slot 0) in ESP-IDF. SD card is not used in this example.

Build
bash
idf.py set-target esp32p4
idf.py build
Important: After the first build, replace audio_setup.c with audio_setup_patched.c (see above), then rebuild.

Flash
bash
idf.py -p COM_PORT flash monitor
Expected Log
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
Notes
C5 firmware: ESP-Hosted 3.0.9 (built from esp-hosted-mcu)

ESP-IDF: v5.5.2

To build C5 firmware, use examples/wifi/sta/cp from the esp-hosted-mcu repository

Links
ESP-Hosted-MCU

ESP-IDF

espressif/esp_codec_dev

