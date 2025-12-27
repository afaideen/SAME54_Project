# SAME54_Project — Bare-Metal Firmware (ATSAME54P20A, SAME54 Xplained Pro)

## 1. Overview
Bare-metal firmware for the **Microchip ATSAME54P20A** on the **SAME54 Xplained Pro** board, built in **MPLAB X** with **XC32 (pic32c)** headers (`sam.h`) and direct `*_REGS` register access.

This repo is a **bring-up + validation demo** that exercises the following on real hardware:

- **Clock and performance bring-up**  
  XOSC0 12 MHz → DPLL0 → **CPU 120 MHz**, flash wait states configured, MPU policy applied, and (optionally) cache handling logged.

- **Timing and profiling**  
  **SysTick 1 ms** tick + **DWT CYCCNT** enabled for fine timing measurements.

- **Concurrent LED blinking using non-blocking delay**  
  The runtime loop keeps LED0 blinking while other work (logging, button handling, and optional QSPI activity) continues, using a **non-blocking DelayMsAsync** style timer.

- **RTCC wall clock**  
  RTCC enabled and used to prepend wall-clock timestamps to logs.

- **UART console on EDBG VCOM**  
  **SERCOM2** (PB24 RX, PB25 TX) with **DMA TX logging** for non-blocking prints.

- **QSPI external flash demonstration**  
  External QSPI NOR flash is initialized in **memory-mapped QUAD mode** and validated by:
  - QSPI SCK configured for **high-speed operation (~30 MHz)** (see QSPI diagnostic at boot)
  1) JEDEC ID read + diagnostic print
  2) **SST26 driver path** used for erase/write/read demo
  3) writing a small config object and reading it back with integrity checks

- **SW0 button interaction**  
  **SW0 (PB31)** is used in the runtime loop to **toggle the LED blink rate interchangeably** (slow ↔ fast) as part of the demo.

---

## 2. Table of Contents
- 1. Overview
- 3. Board Profile Overview
  - 3.1 Power and Debug
  - 3.2 User IO
  - 3.3 UART Console
  - 3.4 External QSPI Flash
  - 3.5 Crystals and Clock Sources
- 4. What You Should See
- 5. Toolchain and Packs
- 6. Project Structure
- 7. Build and Run
- 8. Firmware Flow
- 9. Configuration Switches
- 10. Notes on Cache and MPU
- 11. Firmware Code Flow Diagram
- 12. License
- 13. Credits

---

## 3. Board Profile Overview (SAME54 Xplained Pro)
This firmware targets the **Microchip SAME54 Xplained Pro** (ATSAME54P20A) and uses the board’s default on-board wiring for debug and user IO. The project assumes the **EDBG** debugger is present and provides both programming debug and a virtual COM port.

### 3.1 Power and Debug
- EDBG USB provides programming debug via SWD and exposes a VCOM UART
- SWD pins are PA30 SWCLK and PA31 SWDIO
- SWO trace is PB30 optional high speed debug output

### 3.2 User IO
- LED0 is PC18 active low
  - LED0_On drives low and LED0_Off drives high
- SW0 button is PB31 active low
  - Internal pull up enabled

### 3.3 UART Console (EDBG VCOM)
- SERCOM2 UART routed to EDBG Virtual COM
  - RX is PB24
  - TX is PB25
- Default terminal settings are 115200 8N1
- DMA backed TX logging used for non blocking prints

### 3.4 External QSPI Flash (on board NOR)
- External QSPI NOR flash connected to SAME54 QSPI peripheral
- QSPI wiring
  - PA08 to PA11 are QSPI IO0 to IO3
  - PB10 is QSCK
  - PB11 is QCS
- Firmware supports memory mapped QUAD mode and prints JEDEC and diagnostic info at boot
- QSPI clocking is configured for **high-speed SCK (~30 MHz)** (example BAUD=1 shown in boot diagnostics)
- Demo uses **SST26** driver path for erase and config object write read validation
- QSPI AHB base used by this project is 0x04000000 and mapped region is 16MB

### 3.5 Crystals and Clock Sources
- Main crystal XOSC0 is 12 MHz on PB22 XIN1 and PB23 XOUT1
- 32 kHz crystal XOSC32K is 32.768 kHz on PA00 XIN32 and PA01 XOUT32 used for RTCC low power domain

---

## 4. What You Should See (Sample Log)
The QSPI diagnostic includes the configured QSPI SCK rate, typically **around 30 MHz** in this demo build.

```
QSPI: JEDEC ID = 0x4326BF

================ CLOCK OVERVIEW ================
CPU Clock      : 120.000 MHz
CPU Source     : DPLL0
DPLL0 Output   : 120.000 MHz
DPLL0 Ref      : 1.000 MHz (GCLK2 = DFLL48M / 48)
GCLK0 Divider  : 1
CPU Divider    : 1
GCLK1 (Periph) : 120.000 MHz (DIV=1)
SysTick        : 1.000 ms tick
DWT CYCCNT     : ENABLED
RTCC           : ENABLED
===============================================

=============== QSPI DIAGNOSTIC ===============
QSPI Peripheral : ENABLED
QSPI Mode       : MEMORY
QSPI I/O Mode   : QUAD
QSPI BAUD       : BAUD=1  (~30.000 MHz)
Mem Opcode      : 0xAF
Mem Width       : 4-4-4
JEDEC ID        : 0xBF 26 43
Flash Detected  : VALID
=================================================

[SYS] CMCC=OFF, QSPI_MPU_NC=ON, QSPI_AHB=0x04000000, REGION=16MB
```

---

## 5. Toolchain and Packs (Tested Baseline)
This project was created in **MPLAB X v5.40** and validated with:
- XC32 v4.50 pic32c
- SAME54_DFP v3.5.87
- CMSIS v5.8.0

Tip: When opening in newer MPLAB X, avoid auto upgrading packs unless you intend to update register symbols across the codebase.

---

## 6. Project Structure
```
SAME54_Project/
├─ SAME54_Project.X/
└─ src/
   ├─ main.c
   ├─ common/
   │  ├─ board.c / board.h
   │  ├─ cpu.c / cpu.h
   │  ├─ systick.c / systick.h
   │  └─ delay.c / delay.h
   └─ drivers/
      ├─ uart.c / uart.h
      ├─ uart_dma.c / uart_dma.h
      ├─ rtcc.c / rtcc.h
      └─ qspi/
         ├─ qspi_hw.c / qspi_hw.h
         ├─ qspi_flash.c / qspi_flash.h
         ├─ n25q/
         └─ sst26/
```

---

## 7. Build and Run
1. Open SAME54_Project.X in MPLAB X
2. Select compiler XC32 v4.50
3. Build and program using EDBG
4. Open serial terminal at 115200 8N1
5. Observe clock overview, QSPI diagnostic, RTCC time, QSPI tests, and periodic LED logs
6. Press **SW0** to toggle blink rate between slow and fast

---

## 8. Firmware Flow (High Level)
8.1 SystemConfigPerformance
- flash wait states
- cache and MPU policy including QSPI region attributes
- XOSC0 and DPLL0 to 120 MHz
- GCLK0 and MCLK setup

8.2 board_init
- LED and SW pin config
- SysTick 1ms
- UART2 and DMA logging
- RTCC init if enabled
- QSPI init and JEDEC detection if enabled
- diagnostic printouts

8.3 main loop
- SW0 press toggles blink rate
- LED0 toggles and logs are scheduled using a **non-blocking delay timer** (no busy-wait loops)
---

## 9. Configuration Switches (Feature Gating)
Features enabled via compile time defines:
- BOARD_ENABLE_RTCC enables RTCC init and time prints
- USE_QSPI_FLASH enables QSPI init flash diagnostics and tests

---

## 10. Notes on Cache and MPU
The project prints a runtime policy line such as:
```
[SYS] CMCC=OFF, QSPI_MPU_NC=ON, QSPI_AHB=0x04000000, REGION=16MB
```
Meaning:
- CMCC cache currently OFF
- QSPI memory region forced non cacheable via MPU to avoid coherency issues during memory mapped reads and writes

---

## 11. Firmware Code Flow Diagram
```mermaid
flowchart TD
  A[Reset] --> B[SystemConfigPerformance\nFlash wait states\nCache and MPU policy\nClock setup to 120MHz]

  B --> C[board_init]

  C --> C1[GPIO init\nLED0 output active low\nSW0 input pull up]
  C --> C2[SysTick init 1ms\nDWT cycle counter enable]
  C --> C3[UART2 init on SERCOM2\nDMA TX logging init]

  C3 --> D{RTCC enabled}
  D -- Yes --> D1[RTCC init\nSync from build time]
  D -- No --> E
  D1 --> E

  E{QSPI enabled}
  E -- Yes --> E1[QSPI pin init\nQSPI memory mode quad\nRead JEDEC ID\nPrint QSPI diagnostic]
  E -- No --> F

  E1 --> G[QSPI config object demo\nChip erase\nWrite config\nRead back\nVerify integrity]
  G --> F

  F[Print firmware banner\nPrint system policy line]

  F --> H[Main loop]

  H --> S{SW0 pressed}
  S -- Yes --> T[Toggle blink rate\nslow to fast or fast to slow]
  S -- No --> I

  T --> I{Delay elapsed}
  I -- No --> H
  I -- Yes --> J[Toggle LED0\nLog timestamp and delta]
  J --> H
```
---

## 12. License
Copyright (c) 2025. All rights reserved

---

## 13. Credits
Board Microchip SAME54 Xplained Pro  
DFP headers Microchip SAME54 DFP sam.h
