# C5 Firmware (ESP-Hosted 3.0.9)

Pre-built binary for **ESP32-C5** coprocessor.

## File

- `eh_cp_wifi_sta.bin` — ESP-Hosted firmware v3.0.9 (SDIO transport)

## Flashing via UART

### 1. Connect UART to C5

Use the **J5** connector on WT99P4C5-S1:

| J5 Pin | USB-UART Adapter |
|--------|------------------|
| TXD    | RXD              |
| RXD    | TXD              |
| GND    | GND              |
| BOOT   | GND (pull low)   |
| EN(P4) | GND (pull low)   |

**Important:** Pull **EN of ESP32-P4 to GND** to prevent interference during flashing.

### 2. Put C5 into download mode

- Pull **BOOT (GPIO28)** to GND.
- Power on the board via USB Type-C.
- C5 enters download mode automatically.

### 3. Flash

```bash
esptool.py --chip esp32c5 --port COM_PORT_C5 --baud 460800 erase_flash
esptool.py --chip esp32c5 --port COM_PORT_C5 --baud 460800 write_flash 0x0 eh_cp_wifi_sta.bin
```

Replace `COM_PORT_C5` with your actual port (e.g., `COM3`).

### 4. After flashing

- Disconnect BOOT from GND.
- Disconnect EN(P4) from GND.
- Press EN on the board to reset.

### 5. Verify

Open a serial monitor at **115200 baud** on the C5 UART port. You should see:

```
ESP-Hosted-MCU Slave FW version :: EH-3.0.9
Transport used :: SDIO only
```

## Rebuilding from source (optional)

If you want to build the C5 firmware yourself:

```bash
git clone --recursive https://github.com/espressif/esp-hosted-mcu.git
cd esp-hosted-mcu/examples/wifi/sta/cp
idf.py set-target esp32c5
idf.py build
```

The resulting binary will be at `build/eh_cp_wifi_sta.bin`.