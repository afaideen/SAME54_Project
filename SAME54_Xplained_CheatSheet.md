# SAM E54 Xplained Pro – Master Cheat Sheet

---

## 🔋 Power & VBAT
- **Source priority:** External 5V > Debug USB > Target USB
- **Regulators:** Separate 3.3V for MCU/peripherals and for EDBG/XAM
- **VBAT Select:**
  - Supercap (Pin1–2)
  - MCU 3.3V (Pin2–3)
  - PB03 GPIO (jumper removed)
- **Supercap:** 47 mF, ~45s charge, ~20h backup in ULP
- **Rev A0 caveat:** leakage if VBAT > VDDIO
- **XAM:** Current measurement (100 nA – 400 mA), output in Data Visualizer

---
## 🕒 System Clocks
- **XOSC0 (Main):** 12 MHz external crystal
  - PB22 → XIN1
  - PB23 → XOUT1
- **XOSC1 (RTC):** 32.768 kHz crystal (for RTC and low-power domains)
  - PA00 = XIN32
  - PA01 = XOUT32
- **CPU Clock (DPLL0):** Up to 120 MHz

---
## 🖧 Debug / Communication
- **EDBG USB (Debug USB):**
  - VCOM UART: **SERCOM2 PB24 (RX), PB25 (TX)**
  - SWD: **SWCLK, SWDIO**
  - SWO: **PB30**

---
## 🖧 Debug Interfaces

| Signal   | SAM E54 Pin | Description                  |
|----------|-------------|------------------------------|
| SWCLK    | PA30        | SWD clock                   |
| SWDIO    | PA31        | SWD data                    |
| SWO      | PB30        | Serial Wire Output (trace)  |
| RESETN   | RESET       | Reset line                  |
| VCOM RX  | PB24        | Virtual COM RX (SERCOM2)    |
| VCOM TX  | PB25        | Virtual COM TX (SERCOM2)    |
| DGI SPI  | PC04/PC07/PC05/PD01 | SPI interface       |
| DGI I²C  | PD08/PD09   | I²C interface               |
| DGI GPIOs| PC16, PC17, PC18, PB31 | GPIO event monitoring |

---

## 💡 User Peripherals

| Peripheral | SAM E54 Pin | Notes                        |
|------------|-------------|------------------------------|
| LED0       | PC18        | Active low                   |
| SW0 Button | PB31        | Active low, enable pull-up   |

---

## 🔌 EXT1 Header

| Pin | SAM E54 Pin | Function               |
|-----|-------------|------------------------|
| 3   | PB04        | ADC1/AIN[6]            |
| 4   | PB05        | ADC1/AIN[7]            |
| 5   | PA06        | GPIO (RTS)             |
| 6   | PA07        | GPIO (CTS)             |
| 7   | PB08        | PWM+                   |
| 8   | PB09        | PWM−                   |
| 13  | PA05        | UART RX (SERCOM0)      |
| 14  | PA04        | UART TX (SERCOM0)      |
| 16  | PB27        | SPI MOSI               |
| 17  | PB29        | SPI MISO               |
| 18  | PB26        | SPI SCK                |

---

## 🔌 EXT2 Header

| Pin | SAM E54 Pin | Function               |
|-----|-------------|------------------------|
| 3   | PB00        | ADC0/AIN[12]           |
| 4   | PA03        | ADC0/AIN[1]            |
| 7   | PB14        | PWM+                   |
| 8   | PB15        | PWM−                   |
| 13  | PB17        | UART RX (SERCOM5)      |
| 14  | PB16        | UART TX (SERCOM5)      |
| 16  | PC04        | SPI MOSI               |
| 17  | PC07        | SPI MISO               |
| 18  | PC05        | SPI SCK                |

---

## 🔌 EXT3 Header

| Pin | SAM E54 Pin | Function               |
|-----|-------------|------------------------|
| 3   | PC02        | ADC1/AIN[4]            |
| 4   | PC03        | ADC1/AIN[5]            |
| 7   | PD10        | PWM+                   |
| 8   | PD11        | PWM−                   |
| 13  | PC23        | UART RX (SERCOM1)      |
| 14  | PC22        | UART TX (SERCOM1)      |
| 16  | PC04        | SPI MOSI               |
| 17  | PC07        | SPI MISO               |
| 18  | PC05        | SPI SCK                |

---

## 🚦 Other Key Peripherals
- **CAN:** PB12 (TX), PB13 (RX), PC13 (Standby)
- **Ethernet RMII:**
  - PA14 (REF_CLK), PA17 (TXEN), PA18 (TXD0), PA19 (TXD1)
  - PA12 (RXD1), PA13 (RXD0), PA15 (RXER)
  - PC11 (MDC), PC12 (MDIO), PC20 (CRS_DV), PC21 (RESET)
  - PD12 (INT)
- **QSPI Flash (External NOR):** Micron **N25Q256A**, **256 Mbit** (~32 MB), supports **XIP** via the SAME54 QSPI module.
  - **Signal integrity note:** QSPI traces are routed ~**60 Ω**; set ATSAME54P20A QSPI I/O to **high drive**. Flash drive strength can be tuned in flash config registers to match the ~60 Ω routing.

  **QSPI pin connections:**

  | SAM E54 pin | Function | QSPI flash function | Shared functionality |
  |---|---|---|---|
  | PA08 | QIO0 | Slave In / IO0 | - |
  | PA09 | QIO1 | Slave Out / IO1 | - |
  | PA10 | QIO2 | Write Protect / IO2 | - |
  | PA11 | QIO3 | Hold / IO3 | - |
  | PB10 | QSCK | Clock | - |
  | PB11 | QCS  | Chip Select | - |
- **I²S:** PB16 (SCK0), PB17 (MCK0), PA22 (SDI), PA23 (FS1)

---

## 🔄 SERCOM Mapping (UART / SPI / I²C)

| SERCOM | Configured As | MCU Pins                         | Board Mapping                   |
|--------|---------------|----------------------------------|---------------------------------|
| 0      | UART          | PA04 (TX), PA05 (RX)             | EXT1 pins 14/13                 |
| 0      | SPI/I²C (alt) | PA04/PA05 other mux options      | Not wired by default            |
| 1      | UART          | PC22 (TX), PC23 (RX)             | EXT3 pins 14/13                 |
| 1      | SPI/I²C (alt) | PC22/PC23 other mux options      | Not wired by default            |
| 2      | UART (EDBG)   | PB25 (TX), PB24 (RX)             | Hardwired to EDBG VCOM USB      |
| 2      | SPI/I²C (alt) | PB24/PB25 alt mux                | Possible, reserved for VCOM     |
| 3      | I²C           | PA22 (SDA), PA23 (SCL)           | EXT1 pins 11/12                 |
| 3      | UART/SPI (alt)| PA22/PA23 alt mux                | Possible but conflicts with I²C |
| 4      | SPI           | PB27 (MOSI), PB26 (SCK), PB29 (MISO) | EXT1 pins 16/18/17           |
| 4      | UART (alt)    | PB27/PB26 or other mux options   | Possible                        |
| 5      | UART          | PB16 (TX), PB17 (RX)             | EXT2 pins 14/13                 |
| 5      | SPI/I²C (alt) | PB16/PB17 alt mux                | Possible                        |
| 6      | SPI           | PC04 (MOSI), PC05 (SCK), PC07 (MISO) | EXT2/3 pins 16/18/17         |
| 6      | UART (alt)    | PC04/PC05 alt mux                | Possible                        |
| 7      | I²C           | PD08 (SDA), PD09 (SCL)           | EXT2/3 pins 11/12               |
| 7      | UART/SPI (alt)| PD08/PD09 alt mux                | Possible but conflicts with I²C |

---
