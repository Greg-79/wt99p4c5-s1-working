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

1. **ES7210 conflict**: The stock `ksdiy_p4c5_audio` component tries to init ES7210, which is NOT present on this board. You MUST replace `managed_components/kevincoooool__ksdiy_p4c5_audio/audio_setup.c` with `audio_setup_patched.c` from the project root after the first build, then rebuild.

2. **SD card**: Not tested. There is a known conflict between ESP-Hosted (SDIO slot 1) and SD card (slot 0) in ESP-IDF.

## Build

```bash
idf.py set-target esp32p4
idf.py build