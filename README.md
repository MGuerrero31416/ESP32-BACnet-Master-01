# ESP32 BACnet/IP + MS/TP Air Quality Monitor

ESP32-based BACnet device supporting **BACnet/IP (Wi-Fi)** and **BACnet MS/TP (RS-485)**.

This project started as an air quality monitor using the **Sensirion SEN54** sensor and a **ST7789 touch display**, but it is designed as a reusable ESP-IDF BACnet platform for other sensors, displays, and automation devices.

The firmware exposes sensor values, diagnostics, and control functions as standard BACnet objects.

---

## Features

- BACnet/IP over Wi-Fi
- BACnet MS/TP over RS-485
- Simultaneous BACnet/IP and MS/TP operation
- Change of Value (COV) support
- Persistent BACnet object configuration using ESP32 NVS
- Sensirion SEN54 environmental sensor support
- LVGL graphical display interface
- Modular ESP-IDF component architecture

---

## Hardware

Current reference hardware:

| Device | Description |
|---|---|
| MCU | ESP32 S3 |
| Sensor | Sensirion SEN54 |
| Display | ST7789 170x320 TFT + Touch |
| RS-485 | MAX485 compatible transceiver |

---

## BACnet Objects

Default configuration:

| Object | Quantity | Purpose |
|---|---:|---|
| Analog Value | 7 | Sensor values and configuration |
| Binary Value | 4 | Commands |
| Analog Input | 4 | Read-only analog points |
| Binary Input | 4 | Diagnostics/status |
| Binary Output | 4 | Control outputs |

---

## SEN54 Default Mapping

| Object | Value |
|---|---|
| AV1 | Temperature |
| AV2 | Relative Humidity |
| AV3 | PM2.5 |
| AV4 | VOC Index |
| AV5 | PM1.0 |
| AV6 | PM4.0 |
| AV7 | Automatic cleaning interval |

---

## SEN54 Control Objects

| Object | Function |
|---|---|
| BV1 | Sensor reset |
| BV2 | Enable/disable measurement |
| BV3 | Manual fan cleaning |
| BV4 | Clear sensor status |

---

## SEN54 Diagnostics

| Object | Status |
|---|---|
| BI1 | Fan failure |
| BI2 | Laser error |
| BI3 | VOC sensor error |
| BI4 | Temperature/Humidity sensor error |

---

# Architecture

The firmware uses ESP-IDF components to keep the BACnet core independent from hardware.

```
main/
└── main.c
```

Application entry point only.

---

## Components

```
components/

├── bacnet_app/
│   BACnet runtime
│   BACnet/IP
│   BACnet MS/TP
│   COV handling
│   Service initialization

├── bacnet_objects/
│   Analog Values
│   Binary Values
│   Analog Inputs
│   Binary Inputs
│   Binary Outputs

├── product_config/
│   User configuration
│   Device identity
│   Network settings
│   Object defaults

├── sensors/
│   Sensor application logic

├── ui/
│   Display
│   Touch
│   LVGL screens

├── networking/
│   Wi-Fi support

├── rs485/
│   RS-485 hardware layer

└── system/
    Boot initialization
    NVS handling
```

---

## Configuration

Project settings are centralized in:

```
components/product_config/User_Settings.c
components/product_config/User_Settings.h
```

Configuration includes:

- BACnet Device ID
- BACnet/IP enable
- BACnet MS/TP enable
- MS/TP MAC address
- Baud rate
- Object names
- Default values

Private credentials can be stored locally using:

```
User_Settings_Private.h
```

This file is ignored by Git and should not be uploaded.

---

## BACnet MS/TP Defaults

| Setting | Default |
|---|---|
| Baud rate | 38400 |
| Max Master | 127 |
| Max Info Frames | 80 |

---

## Build

Requires ESP-IDF.

```bash
idf.py build
idf.py flash monitor
```

---

## Development Notes

The project is designed so new hardware variants can replace:

- sensors
- display/UI
- communication hardware

without modifying the BACnet core.

Future examples:

- ESP32 + SEN54 air monitor
- ESP32-S3 Ethernet I/O controller
- Touchscreen BACnet controller
- Custom BACnet sensor gateways

---

## References

- ESP-IDF Documentation  
https://docs.espressif.com/projects/esp-idf/

- BACnet Stack  
https://github.com/bacnet-stack/bacnet-stack