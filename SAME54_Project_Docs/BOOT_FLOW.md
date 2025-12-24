# BOOT_FLOW.md — Boot and Initialization Flow (SAME54 Xplained Pro)

This document explains the **boot timeline and init order** used in this project, focusing on:
- clock bring-up,
- flash wait states,
- cache enablement,
- SysTick 1ms timebase,
- and when peripherals become safe to use.

It is written for the project’s **bare-metal XC32 SAME54 header model** (`sam.h`, `*_REGS`).

---

## 1. Reset to C runtime startup

### 1.1 Reset entry
After reset, the Cortex-M core:
1. Loads the initial stack pointer from the vector table.
2. Jumps to the `Reset_Handler` (provided by the toolchain/startup files).

### 1.2 C runtime init
Startup code performs the standard C runtime steps:
- copies `.data` from flash to RAM,
- clears `.bss`,
- runs any required C runtime initialization,
- then calls `main()`.

This project’s *real* platform configuration begins at `main()`.

---

## 2. Required init order at a glance

**Mandatory order**
1. `SystemConfigPerformance()`  *(clocks + flash + cache + DWT)*
2. `board_init()`               *(GPIO + SysTick + UART DMA + optional RTCC)*
3. main superloop               *(application behavior)*

Rule:
- No peripheral initialization should happen before `SystemConfigPerformance()`.

---

## 3. SystemConfigPerformance() — performance and clock bring-up

`SystemConfigPerformance()` is the one-shot platform function that establishes a stable high-speed system.

### 3.1 Flash wait states
Before running fast:
- set **NVM flash wait states** to a safe value for the target frequency
- this prevents instruction fetch stalls or invalid reads at high CPU rates

This project sets flash to a conservative high-speed setting (RWS configured for the 120 MHz envelope).

### 3.2 Cache enable
Enable **CMCC cache** early:
- reduces flash fetch latency
- improves instruction throughput at higher clocks

### 3.3 Clock strategy overview
This project uses a **DPLL-based** system clock:

- A reference clock is prepared (derived from DFLL48M path).
- **DPLL0** is configured to produce a high-speed clock domain (~120 MHz).
- Clock generators are then built from DPLL0:
    - **GCLK0**: CPU generator
    - **GCLK1**: peripheral generator (UART depends on this)

The firmware supports CPU operation at **120 MHz** or **60 MHz** via a CPU divider selection.

### 3.4 Generator and divider policy

- **GCLK0** is used as the CPU clock source.
- **MCLK CPU divider** selects the actual CPU rate:
    - divider 1 → 120 MHz CPU
    - divider 2 → 60 MHz CPU

- **GCLK1** is configured so that:
    - **GCLK1 == CPU clock** (project contract)
    - SERCOM2 core clock is routed from GEN1
    - UART baud calculations assume this relationship

### 3.5 Peripheral clock channels
After GCLK generators are stable, `SystemConfigPerformance()` enables:
- peripheral clock channels (PCHCTRL) for required blocks such as:
    - EIC (button input path)
    - SERCOM2 core (UART)
    - SERCOM3 core (reserved/optional)

### 3.6 Bus mask enabling
The function also programs MCLK masks to ensure buses and blocks are clocked.

Important note:
- these masks are written as fixed constants, so future peripherals not included may need the mask values updated.

### 3.7 DWT cycle counter enable (microsecond delta support)
`SystemConfigPerformance()` enables:
- `CoreDebug->DEMCR.TRCENA`
- `DWT->CYCCNT` running

This is used by the UART DMA logger for fine delta timing.

### 3.8 Defensive RTCC shutdown at boot
To avoid “leftover RTC state” across resets, the platform init:
- disables RTC interrupts in NVIC,
- disables and resets RTC MODE2 block,
- clears flags and waits for sync.

RTCC is later initialized cleanly in `board_init()` if enabled.

---

## 4. board_init() — board peripherals and timebase

`board_init()` is the BSP layer that makes the system “usable”.

### 4.1 GPIO init
- LED0: **PC18 output**, active-low
- SW0: **PB31 input**, pull-up enabled, active-low

### 4.2 SysTick 1ms init
`board_init()` configures the core SysTick to generate a **1ms tick**:
- `systick_ms` increments in `SysTick_Handler()`
- `millis()` returns the monotonic ms count

This ms timebase is used for:
- non-blocking scheduling,
- debounce timing,
- log timestamps.

### 4.3 UART logging stack init
`board_init()` initializes the logging path:

1. `UART2_DMA_Init()`:
    - calls `UART2_Init()` to configure SERCOM2 UART pins and registers
    - initializes DMAC base/writeback descriptors
    - configures **DMAC channel 0** for SERCOM2 TX pacing trigger
    - enables DMAC channel interrupt

After this step:
- **UART2 DMA logging is safe to call**.

### 4.4 Optional RTCC init and seed
If enabled:
- `RTCC_Init()` configures RTC MODE2 calendar clock
- `RTCC_SyncFromBuildTime()` seeds calendar time from `__DATE__` and `__TIME__` when RTC is not already plausible

RTCC is used only for **seconds-resolution calendar prefix** in logs.
Sub-second deltas come from SysTick and DWT.

### 4.5 Boot banner and clock overview
Once UART exists, `board_init()` prints:
- clock overview (derived values)
- firmware banner

---

## 5. Main loop runtime behavior

After `SystemConfigPerformance()` and `board_init()` complete, `main()` enters the superloop.

Typical runtime behaviors:
- poll switch with debounce timing based on `millis()`
- update LED blink period (500 ms normal, 100 ms on mode changes depending on the app logic)
- periodic tasks use `DelayMsAsync()` for non-blocking timing
- log messages are enqueued via `UART2_DMA_Log()` and transmitted by DMAC in the background

---

## 6. Interrupt involvement

### 6.1 SysTick interrupt
Frequency:
- every 1 ms

Handler responsibility:
- increment `systick_ms`

Impact:
- all ms-based scheduling depends on this tick
- if interrupts are disabled for long periods, ms time “pauses” and delays stretch

### 6.2 DMAC channel interrupt
Fires when a UART TX DMA transfer:
- completes, or
- errors

Handler responsibility:
- clear flags
- mark DMA idle
- start the next queued log message

This makes logging non-blocking:
- main enqueues,
- DMAC IRQ drains.

---

## 7. Why this init order matters

### 7.1 Clock before SysTick
SysTick tick rate depends on CPU clock.
Therefore:
- clocks must be stable before SysTick init,
- SysTick must be initialized with the correct CPU frequency.

### 7.2 Clock contract for UART
SERCOM2 baud correctness depends on:
- SERCOM2 core clock being routed from GEN1,
- GEN1 frequency matching what the baud calculation expects.

This project enforces:
- **GCLK1 equals CPU clock**.

### 7.3 Flash wait states before speed
Running fast without proper NVM wait states risks unstable execution.
So:
- flash wait state configuration happens before high-speed clock switching.

### 7.4 Cache early for performance
Cache reduces flash fetch latency and improves instruction performance,
especially when running from high-frequency clocks.

---

## 8. Known assumptions and limits

- SysTick provides a stable 1ms tick (correct CPU hz).
- DWT CYCCNT is enabled and increments at CPU clock rate.
- DWT is 32-bit and wraps quickly:
    - ~35.8 s at 120 MHz
    - ~71.6 s at 60 MHz
- RTC uses 1-second resolution; sub-second timing is not provided by RTCC.
- RTCC oscillator-ready waits may hang if the selected source is not present or not starting.

---

## 9. Minimal “safe to call” timeline

- Before `SystemConfigPerformance()`:
    - do not touch peripherals

- After `SystemConfigPerformance()`:
    - clocks and DWT are valid
    - peripherals are not yet configured

- After SysTick init in `board_init()`:
    - `millis()` is valid

- After `UART2_DMA_Init()` in `board_init()`:
    - `UART2_DMA_Log()` is valid

- After RTCC init (optional):
    - calendar timestamps become valid (if RTC deemed plausible or seeded)

---
