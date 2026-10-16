# P4 Firmware (Full Flash Dump)

Pre-built full flash dump for **ESP32-P4** main MCU.

## File

- `p4_full_flash.bin` — 16 MB full flash dump (bootloader + partition table + app + PHY)

## Flashing

```bash
esptool.py --chip esp32p4 --port COM_PORT --baud 460800 write_flash 0x0 p4_full_flash.bin
```

Replace `COM_PORT` with your actual port.

## After flashing

The NVS partition is cleared (Wi-Fi credentials are not included). To connect to Wi-Fi, edit `main/main.c`:

```c
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"
```

Then rebuild and reflash, or configure via `idf.py menuconfig`.

## Verify

Open a serial monitor at **115200 baud** on the P4 UART port. You should see:

```
I (xxx) AUDIO: Found I2C device at address: 0x18
I (xxx) AUDIO: ES8311 duplex ready (DAC+ADC)
I (xxx) eh_init_evt: slave chip id: 0x17 (esp32c5)
I (xxx) MAIN: got ip:192.168.x.x
I (xxx) MIC: Peak: 13472, RMS: 5423.6
```