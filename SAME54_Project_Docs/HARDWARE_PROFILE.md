# HARDWARE_PROFILE.md — SAME54 Xplained Pro (ATSAME54P20A) for this project

This document captures the **board-level wiring assumptions** and **clock/peripheral choices** used by the firmware in this repo. It is intentionally concise and practical (what pin maps to what, what clocks are assumed, what “active-low” means, etc.).

---

## 1. Target board and MCU

- Board: **SAM E54 Xplained Pro**
- MCU: **ATSAME54P20A**
- I/O voltage: **3.3V**
- On-board debug interface: **EDBG** (provides USB programming/debug and a Virtual COM Port)

---

## 2. User peripherals used by this firmware

| Function | MCU Port/Pin | Electrical | Notes |
|---|---:|---|---|
| LED0 | **PC18** | **Active-low** | `0 = ON`, `1 = OFF` |
| SW0 Button | **PB31** | **Active-low** | Internal pull-up enabled; pressed reads as `0` |

### LED0 behavior
- LED0 is treated as **active-low**.
- “LED ON” means the code drives PC18 **low**.

### SW0 behavior
- SW0 is treated as **active-low** with pull-up.
- Pull-up is enabled by setting input config + enabling PULLEN and setting OUT=1 for that pin.
- “Pressed” means PB31 reads **low**.

---

## 3. Console UART (EDBG Virtual COM)

This project logs over the on-board EDBG Virtual COM Port using **SERCOM2 USART**.

| UART Signal | SERCOM | MCU Pin | Direction | Notes |
|---|---|---:|---|---|
| VCOM RX (to MCU) | SERCOM2 | **PB24** | MCU RX | From EDBG |
| VCOM TX (from MCU) | SERCOM2 | **PB25** | MCU TX | To EDBG |

### UART usage model
- TX is primarily **DMAC-driven** (non-blocking logging).
- RX is not a major feature in this project’s architecture (logging oriented).

---

## 4. DMAC usage (UART logging)

- DMAC is used only for **UART TX logging**.
- **DMAC channel 0** is reserved for SERCOM2 TX in this design.
- Trigger is peripheral-paced: **SERCOM2 TX ready trigger**.
- Transfer unit: **1 byte per trigger**, memory → SERCOM2 DATA register.
- Completion ISR: **DMAC channel 0 interrupt** is used to “send next queued log”.

Implications:
- UART logging is **non-blocking** (main enqueues, DMAC drains).
- If interrupts are disabled too long, the queue can fill and logs may drop.

---

## 5. Clock profile used by this firmware

This firmware assumes a “performance clock tree” with a 120 MHz DPLL base and supports a CPU divider for 60/120 operation.

### 5.1 Core clocking
- **DPLL0 target**: **120 MHz**
- CPU clock options:
    - **120 MHz** (divider 1)
    - **60 MHz** (divider 2)

### 5.2 Generator usage
- **GCLK0**: CPU clock generator (drives core)
- **GCLK1**: peripheral clock generator used for SERCOM2
    - Project assumption: **GCLK1 == CPU clock** (important for UART baud correctness)

### 5.3 1ms system tick
- SysTick is configured for **1ms interrupt tick**
- `millis()` is derived from that tick and is the base for:
    - non-blocking timers (blink scheduling)
    - switch debounce timing
    - log timestamps (ms field)

---

## 6. RTCC (calendar clock) profile

RTCC is used as an optional “human time” source for log prefixes.

- RTC mode: **MODE2 (calendar clock)**
- Clock source policy:
    - Prefer external **XOSC32K** domain routed as a **1K clock** if enabled on the board
    - Optionally allow **ULP1K** (internal low power) in builds that do not rely on crystal startup
- Prescaler: **/1024** to get **~1 Hz** calendar tick from a ~1024 Hz 1K source

Notes:
- RTCC in this project is used for **seconds resolution** timestamping only.
- Sub-second deltas come from **DWT CYCCNT** (not RTCC).

Year range assumption:
- Calendar year is treated as **2000–2063** (hardware field limit).

---

## 7. Electrical and “gotcha” reminders

- **Active-low LED**: “ON” means output low.
- **Active-low switch**: pressed means input low; pull-up required.
- If external 32k crystal does not start (or is not populated in a custom board variant):
    - RTCC init that waits for oscillator-ready can hang.
    - Use the internal **ULP1K** option or add timeouts if you plan to support crystal-less variants.
- UART correctness depends strongly on:
    - correct SERCOM2 core clock routing (GCLK1 expected),
    - correct CPU clock setting (60 vs 120).

---

## 8. Toolchain and register model expectation

This project is written against the Microchip SAME54 DFP headers:
- Includes `sam.h`
- Uses `*_REGS` peripheral access style (e.g., `SERCOM2_REGS`, `GCLK_REGS`, `MCLK_REGS`)

This is not Harmony PLIB style and not CMSIS pointer-peripheral style.

---

## 9. Quick bring-up checklist

1. Confirm LED0 (PC18) toggles at expected rate.
2. Confirm SW0 (PB31) reads high idle, low when pressed.
3. Confirm UART logs appear on EDBG VCOM:
    - correct COM port
    - expected baud
4. Confirm logging remains responsive while LED blinking continues (DMAC TX is working).
5. If RTCC is enabled:
    - verify timestamps show real date/time after sync-from-build or manual set.

---
