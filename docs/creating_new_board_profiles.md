# Creating New Board Profiles

This project separates **hardware configuration** from the BACnet application logic.

Use board profiles when you want to build the same BACnet firmware core for different hardware, for example:

- PMS5003 instead of SEN54
- Different displays or touch panels
- External Ethernet modules
- Different RS-485 transceivers
- Different ESP32 boards or pinouts

The goal is to avoid modifying `main.c`, `bacnet_app`, or `bacnet_objects` for each hardware variant.

---

## 1. Current Concept

The active hardware profile is selected through:

```c
components/board_config/board_config.h
```

Each board profile defines hardware details such as:

- GPIO pins
- UART ports
- I2C ports
- SPI hosts
- display type
- touch controller
- RS-485 driver pins
- optional Ethernet interface
- enabled onboard peripherals

Example profile:

```c
components/board_config/board_spotpear_lcd_1_9.h
```

---

## 2. Recommended Rule

Keep these responsibilities separate:

| Layer | Purpose |
|---|---|
| `board_config` | Hardware pins, buses, and board capabilities |
| `product_config` | BACnet device identity, object names, defaults, network settings |
| `sensors` | Sensor application logic |
| `ui` | Display and touch UI |
| `rs485` | RS-485 hardware driver |
| `networking` | Wi-Fi or Ethernet support |
| `bacnet_app` | BACnet runtime and services |
| `bacnet_objects` | BACnet AV/BV/AI/BI/BO object implementation |
| `main` | Startup orchestration only |

Do not put board-specific GPIOs in `main.c`, `bacnet_app`, or `bacnet_objects`.

---

## 3. Adding a New Board Profile

### Step 1 — Create a new board header

Example:

```text
components/board_config/board_my_new_board.h
```

Start from the existing board profile and copy only the hardware definitions.

Example:

```c
#pragma once

#define BOARD_NAME "My New ESP32 Board"

/* SEN54 / external sensor I2C */
#define BOARD_SEN54_I2C_PORT       0
#define BOARD_SEN54_I2C_SDA_PIN    42
#define BOARD_SEN54_I2C_SCL_PIN    45

/* RS-485 */
#define BOARD_RS485_UART_PORT      2
#define BOARD_RS485_TX_PIN         16
#define BOARD_RS485_RX_PIN         17
#define BOARD_RS485_DE_RE_PIN      5

/* Display */
#define BOARD_DISPLAY_ENABLED      1
#define BOARD_DISPLAY_TYPE_ST7789  1
#define BOARD_DISPLAY_SPI_HOST     1
#define BOARD_DISPLAY_MOSI_PIN     13
#define BOARD_DISPLAY_CLK_PIN      10
#define BOARD_DISPLAY_CS_PIN       12
#define BOARD_DISPLAY_DC_PIN       11
#define BOARD_DISPLAY_RST_PIN      9
#define BOARD_DISPLAY_BL_PIN       14

/* Touch */
#define BOARD_TOUCH_ENABLED        1
#define BOARD_TOUCH_I2C_PORT       0
#define BOARD_TOUCH_SDA_PIN        47
#define BOARD_TOUCH_SCL_PIN        48
#define BOARD_TOUCH_I2C_ADDR       0x15
```

---

### Step 2 — Select the new board profile

Edit:

```text
components/board_config/board_config.h
```

Example:

```c
#pragma once

#define BOARD_SPOTPEAR_LCD_1_9 0
#define BOARD_MY_NEW_BOARD     1

#if BOARD_SPOTPEAR_LCD_1_9
#include "board_spotpear_lcd_1_9.h"
#elif BOARD_MY_NEW_BOARD
#include "board_my_new_board.h"
#else
#error "No board profile selected"
#endif
```

Only one board profile should be active at a time.

---

### Step 3 — Build

Run:

```bash
idf.py build
```

Fix only compile errors related to the new board profile.

---

## 4. Using PMS5003 Instead of SEN54

The PMS5003 uses UART, while SEN54 uses I2C.

A PMS5003 board profile should define:

```c
#define BOARD_PMS5003_ENABLED      1
#define BOARD_SEN54_ENABLED        0

#define BOARD_PMS5003_UART_PORT    1
#define BOARD_PMS5003_RX_PIN       18
#define BOARD_PMS5003_TX_PIN       19
#define BOARD_PMS5003_SET_PIN      21
```

The sensor application layer should decide which sensor app to start based on board or product configuration.

Recommended approach:

```text
components/sensors/
├── sen54_app.c
├── sen54_app.h
├── pms5003_app.c
└── pms5003_app.h
```

Do not modify BACnet object code for each sensor. Sensors should publish values into the existing BACnet objects.

Example mapping for PMS5003:

| BACnet Object | PMS5003 Value |
|---|---|
| AV1 | PM1.0 |
| AV2 | PM2.5 |
| AV3 | PM10 |
| BI1 | Sensor communication fault |

If the PMS5003 board does not provide temperature, humidity, or VOC, leave unused AV objects disabled, renamed, or set to invalid values through `product_config`.

---

## 5. Using a Different Display or UI

A display profile should define:

```c
#define BOARD_DISPLAY_ENABLED       1
#define BOARD_DISPLAY_TYPE_ST7789   1
#define BOARD_DISPLAY_WIDTH         320
#define BOARD_DISPLAY_HEIGHT        170
#define BOARD_DISPLAY_TOUCH_ENABLED 1
```

For a different display, create a new UI/display implementation rather than editing the BACnet core.

Recommended future structure (example):

```text
components/ui/
├── ui_manager.c
├── ui_manager.h
├── display_port.h
├── display_st7789.cpp
├── display_gc9a01.cpp
└── screens/
    ├── screen_main.cpp
    ├── screen_controls.cpp
    └── screen_diagnostics.cpp
```

The UI should read application values and BACnet object values, but it should not own sensor logic or BACnet transport logic.

If the display resolution changes, keep layout constants inside the UI layer.

Do not hardcode display dimensions inside sensor or BACnet modules.

---

## 6. Adding an External Ethernet Board

External Ethernet should be added as a network interface option, not as a change to BACnet objects.

Possible Ethernet hardware:

- W5500 SPI Ethernet
- LAN8720 RMII Ethernet
- Other ESP-IDF-supported Ethernet PHYs

A W5500 board profile may define:

```c
#define BOARD_ETHERNET_ENABLED      1
#define BOARD_ETHERNET_TYPE_W5500   1

#define BOARD_W5500_SPI_HOST        2
#define BOARD_W5500_MOSI_PIN        13
#define BOARD_W5500_MISO_PIN        14
#define BOARD_W5500_CLK_PIN         15
#define BOARD_W5500_CS_PIN          16
#define BOARD_W5500_INT_PIN         12
#define BOARD_W5500_RST_PIN         39
```

Recommended structure:

```text
components/networking/
├── wifi_helper.c
├── wifi_helper.h
├── ethernet_w5500.c
├── ethernet_w5500.h
├── network_manager.c
└── network_manager.h
```

Long-term goal:

```c
network_manager_start();
```

The BACnet/IP layer should use the active network interface without knowing whether it is Wi-Fi or Ethernet.

Do not duplicate BACnet/IP logic for Wi-Fi and Ethernet.

---

## 7. Using a Different RS-485 Converter

Most RS-485 modules use the same UART signals:

- TX
- RX
- DE/RE direction control

For a MAX485-style converter:

```c
#define BOARD_RS485_TYPE_MAX485     1
#define BOARD_RS485_UART_PORT       2
#define BOARD_RS485_TX_PIN          16
#define BOARD_RS485_RX_PIN          17
#define BOARD_RS485_DE_RE_PIN       5
#define BOARD_RS485_DE_ACTIVE_HIGH  1
```

For converters with automatic direction control:

```c
#define BOARD_RS485_AUTO_DIRECTION  1
#define BOARD_RS485_DE_RE_PIN       -1
```

The RS-485 driver should use board macros and hide the hardware differences from BACnet MS/TP.

BACnet MS/TP should not directly manipulate GPIO pins.

---

## 8. Checklist for a New Board

Before creating a new board profile, collect:

```text
Board name:
ESP32 variant:
Flash size:
PSRAM:
Display:
Touch controller:
Sensor:
RS-485 converter:
Ethernet module:
UART pins:
I2C pins:
SPI pins:
Reserved pins:
Boot strapping pins:
USB/JTAG pins:
Onboard peripherals:
```

Then check:

- Are any pins shared?
- Are GPIOs valid for input/output?
- Are any pins bootstrapping pins?
- Are any pins used by flash, PSRAM, USB, display, touch, or SD card?
- Does the board already use I2C0 or I2C1?
- Does the board already use SPI2 or SPI3?
- Does the board already use UART0 for logs?

---

## 9. Build and Test Checklist

After adding a new board profile:

```bash
idf.py build
idf.py flash monitor
```

Verify:

1. Boot log is clean.
2. Display starts, if enabled.
3. Touch works, if enabled.
4. Sensor reads correctly.
5. Wi-Fi or Ethernet connects.
6. BACnet/IP discovery works.
7. BACnet MS/TP discovery works.
8. BACnet objects are readable.
9. Writable BV/BO objects work.
10. No task creation failures.
11. No repeated I2C/UART/SPI errors.
12. No GPIO conflict warnings.

---

## 10. What Not To Do

Do not:

- Create a separate Git branch for every hardware variant.
- Copy the whole project for every board.
- Put hardware pins in `main.c`.
- Put display logic in `bacnet_app`.
- Put BACnet transport logic in sensor drivers.
- Put Wi-Fi passwords in committed source files.
- Modify BACnet object numbering for each board unless intentionally changing the product model.

---

## 11. Suggested Future Structure

As the project grows, the preferred structure is:

```text
components/
├── board_config/
│   ├── board_config.h
│   ├── board_spotpear_lcd_1_9.h
│   ├── board_pms5003_display.h
│   ├── board_w5500_gateway.h
│   └── board_waveshare_poe_8di_8do.h
│
├── product_config/
│   ├── User_Settings.c
│   ├── User_Settings.h
│   └── User_Settings_Private.h.example
│
├── sensors/
│   ├── sen54_app.c
│   ├── pms5003_app.c
│   └── sensor_manager.c
│
├── ui/
│   ├── ui_manager.c
│   ├── display_port.h
│   └── screens/
│
├── networking/
│   ├── wifi_helper.c
│   ├── ethernet_w5500.c
│   └── network_manager.c
│
├── rs485/
│   └── mstp_rs485.c
│
├── bacnet_app/
├── bacnet_objects/
└── system/
```

---

## 12. Recommended Development Flow

For each new hardware variant:

1. Create a new board profile.
2. Build.
3. Flash.
4. Test hardware basics.
5. Only then add or modify sensor/UI/network code.
6. Keep BACnet core unchanged.
7. Commit and tag stable milestones.

Example tags:

```bash
git tag v2.1.0-board-config-baseline
git tag v2.2.0-pms5003-profile
git tag v2.3.0-w5500-profile
```
