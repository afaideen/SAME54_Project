# ARCHITECTURE

This document explains the architecture of the **SAME54 Project** for the **Microchip SAM E54 Xplained Pro** board.

It is written to be **blog-ready** and **repo-ready**: clear intent, explicit data flow, and easy extension points.

---

## 1. Project intent

This repository is a **bare metal firmware baseline** that focuses on:

- Deterministic clock bring up to a known performance point
- A small set of board peripherals that prove the platform is alive
- A non blocking execution model that scales without an RTOS
- A logging path that stays usable as the system grows

The code style intentionally uses a **PIC32 like GPIO abstraction** for readability while keeping **direct register control** for clocks and peripherals.

---

## 2. Hardware target

Board: **SAM E54 Xplained Pro**

Key pins used by this project:

- LED0 on PC18 active low
- SW0 on PB31 active low with internal pull up
- VCOM UART on SERCOM2 pins PB24 RX and PB25 TX
- Optional SWO on PB30

QSPI external flash on the board is typically Micron N25Q256A or compatible.
The project also targets high speed QSPI operation at about **30 MHz** to distinguish it from slower demo configurations.

---

## 3. High level system blocks

### 3.1 Core bring up
- Clock tree and CPU divider selection
- Flash wait state configuration
- CMCC instruction cache configuration (compile-time selectable)

### 3.2 Timing base
- SysTick provides a 1 ms tick
- Optional DWT cycle counter provides fine grained profiling

### 3.3 Board layer
- GPIO macros for LED0 and SW0
- Debounced button event detection

### 3.4 Logging
- UART based log output via the EDBG VCOM port
- DMA driven transmit to reduce CPU stalls and keep logs usable during busy loops

### 3.5 QSPI
- Flash detect via JEDEC read
- QSPI configured for high speed clocking where supported

---

## 4. Boot and init order

The init order is designed to make failures obvious and recoverable.

1. **Reset entry**
2. **SystemConfigPerformance**
   - Select main clock source and setup DPLL
   - Set CPU divider and GCLK0
   - Configure flash wait states
   - Configure CMCC cache (enabled only if `CPU_ENABLE_CMCC_CACHE=1`, default is disabled)
   - Configure MPU region to keep the QSPI AHB window non-cacheable
3. **BOARD_Init**
   - Configure LED0 output and default state
   - Configure SW0 input and pull up
4. **SysTick_Init**
   - Start 1 ms periodic tick
5. **Optional DWT_Init**
   - Enable cycle counter for profiling and timestamp resolution
6. **UART2 DMA init**
   - Configure SERCOM2 pins and baud
   - Configure DMAC channel for TX
7. **QSPI init**
   - Configure QSPI pins and clock
   - Read JEDEC ID and validate
8. Enter **main loop scheduler**

This order ensures that you have a stable clock and safe flash timing **before** enabling fast peripherals and before producing heavy logs.

---

## 5. Execution model

The project uses a **single superloop** with **non blocking tasks**.

Principles:

- No task is allowed to spin wait for time
- Any action that needs time uses a timestamp and returns
- ISR work is kept minimal and only posts events or completes DMA transfers

This makes it easy to add more features without introducing hidden coupling.

### 5.1 Task examples

- **LED task**
  - Toggles LED0 when its next scheduled timestamp has arrived
  - Period changes when SW0 event is detected

- **Button task**
  - Samples SW0
  - Applies debounce window
  - Generates a press event and a single toggle action

- **Logger task**
  - Formats text into a buffer
  - Starts DMA transmit if idle
  - Returns immediately if DMA is busy

- **QSPI task**
  - Performs init checks
  - Runs simple read and write tests if enabled
  - Logs results without blocking the scheduler

---

## 6. Flash and cache policy

### 6.1 Flash wait states
At 120 MHz the firmware sets **RWS=5** and enables AUTOWS.

### 6.2 CMCC cache
The code **configures CMCC**, but cache enable is controlled by a build switch:

- `CPU_ENABLE_CMCC_CACHE=0` (default): CMCC cache **disabled**
- `CPU_ENABLE_CMCC_CACHE=1`: CMCC cache **enabled**

This default was chosen to avoid unintended caching behavior when exercising the **QSPI AHB window**.

### 6.3 QSPI non-cacheable mapping
An MPU region is configured so the QSPI AHB address window is treated as **non-cacheable device memory**.

## 6. Timing and timestamping

### 6.1 Millisecond tick

A 1 ms tick is used as the main time base for:

- DelayMs style scheduling
- Debounce windows
- Periodic background tasks

Wraparound safety:

- Always compute delta using unsigned arithmetic
- Compare elapsed time with `(now - start)` style patterns

### 6.2 Cycle counter

When enabled, DWT CYCCNT supports:

- Profiling code paths
- Measuring ISR overhead
- Deriving microsecond level timing with `cycles / cpu_hz`

---

## 7. UART logging path

### 7.1 Why DMA

DMA transmit is used because:

- Log output can otherwise dominate CPU time
- High rate logs can interfere with timing if transmitted by polling
- DMA keeps the log path predictable and the superloop responsive

### 7.2 Ownership and backpressure

Data ownership rules:

- The DMA engine owns the TX buffer while active
- The logger must not modify the buffer until DMA complete

Backpressure rules:

- If DMA is busy, either drop the message or queue it
- For a baseline project, dropping is acceptable but must be visible in the code

---

## 8. QSPI architecture

### 8.1 Goals

- Confirm board wiring and flash presence
- Run at a distinct speed target around 30 MHz
- Provide a base for future memory mapped or XIP experiments

### 8.2 Typical init flow

- Enable QSPI clock
- Configure QSPI pins and drive strength as needed
- Set baud or divider
- Issue JEDEC read command
- Validate manufacturer and device bytes
- Switch into the desired QSPI mode

---

## 9. Error handling posture

Baseline projects often hide failures.
This one should prefer fail loud behavior.

Recommended pattern:

- Log the failure reason
- Blink LED in an error pattern
- Stay in a safe loop rather than running partially initialized

---

## 10. Extension points

This architecture is designed to accept:

- Low power idle and sleep entry from the main loop
- RTCC alarm scheduling as another non blocking task
- A flash driver layer that supports multiple devices using a function table
- A small event queue if task interactions increase

---

## 11. How to read the code

Start in this order:

1. main application entry
2. SystemConfigPerformance clock bring up
3. board layer init and macros
4. systick and delay helpers
5. UART driver and DMA glue
6. QSPI init and read write routines

---

## 12. Glossary

- GCLK0: main generic clock used for CPU clocking
- DPLL0: digital phase locked loop used for high speed clock generation
- CMCC: cache controller
- SysTick: ARM system timer used for 1 ms tick
- DMAC: DMA controller
- QSPI: quad serial peripheral interface
