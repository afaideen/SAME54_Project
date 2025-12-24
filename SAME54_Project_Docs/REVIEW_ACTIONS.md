# REVIEW_ACTIONS.md — Senior Firmware Review Findings & Actions

This document captures **architectural risks, hidden coupling, and missing hardening** identified during senior-level review of the SAME54 firmware project.  
Actions are prioritized by **severity** and **return on investment (ROI)**.

---

## Severity Levels

- **P0 — Must fix**: Can cause hard hangs, silent corruption, or unrecoverable field failures
- **P1 — Should fix**: High maintenance or portability risk
- **P2 — Nice to fix**: Improves robustness, readability, or future reuse

---

## P0 — Must Fix

### A1. Add bounded timeouts to all wait-forever loops
**Risk**  
Clock, oscillator, or sync failure causes permanent boot hang with no diagnostics.

**Where**
- `cpu.c`
    - DPLL lock wait
    - DFLL ready wait
    - SYNCBUSY waits
- `rtcc.c`
    - XOSC32K ready wait

**Action**
- Add timeout counters to all `while (!(...READY))` loops
- On timeout:
    - fall back to safe clock source
    - set global fault flag
    - blink LED in fault pattern (even if UART not available)

**Benefit**
- Prevents bricked boards
- Allows field diagnostics even when clocks fail

---

### A2. Remove blocking behavior from `board_sw_pressed()`
**Risk**  
Main loop can stall indefinitely if switch is held or shorted.

**Where**
- `board.c`

**Action**
- Replace wait-for-release loop with a non-blocking state machine:
    - IDLE → PRESS_DETECTED → QUALIFIED → WAIT_RELEASE
- No busy wait, no `DelayMs()` inside button logic

**Benefit**
- Preserves cooperative superloop guarantees
- Prevents hidden latency spikes

---

### A3. Consolidate RTCC clock ownership
**Risk**  
Two writers modify `OSC32KCTRL_RTCCTRL`, leading to subtle clock conflicts.

**Where**
- `cpu.c`
- `rtcc.c`

**Action**
- Choose **one module** as RTCC clock owner (preferably `rtcc.c`)
- `cpu.c` should only disable RTC safely, not select clock sources

**Benefit**
- Removes hidden coupling
- Makes RTCC behavior predictable

---

## P1 — Should Fix

### B1. Harden UART DMA error recovery
**Risk**  
Single DMAC fault can permanently stall logging.

**Where**
- `uart_dma.c`

**Action**
- On DMAC TERR:
    - disable channel
    - clear descriptor
    - reinitialize channel state
- Add optional fault counter

**Benefit**
- Prevents silent loss of logs
- Improves field reliability

---

### B2. Replace fixed MCLK mask writes with OR-based enables
**Risk**  
Future peripherals silently disabled by hard-coded masks.

**Where**
- `cpu.c` (MCLK masks init)

**Action**
- Use OR-in enables instead of full overwrite
- Document required masks per peripheral

**Benefit**
- Improves extensibility
- Reduces regression risk

---

### B3. Make clock contract explicit and centralized
**Risk**  
UART silently breaks if GEN1 divider is changed.

**Where**
- `cpu.c`
- `board.h`
- `uart.c`

**Action**
- Add explicit comment block:
    - “SERCOM2 assumes GCLK1 equals CPU clock”
- Optional compile-time assert tying GEN1 divider to CPU divider

**Benefit**
- Prevents accidental clock misconfiguration

---

### B4. Fix DelayMsAsync drift behavior
**Risk**  
Periodic tasks drift under load.

**Where**
- `delay.c`

**Action**
- Change:
    - `t_delay = now`
    - to `t_delay += period`
- Document drift behavior if intentional

**Benefit**
- More stable scheduling for future reuse

---

## P2 — Nice to Fix

### C1. Unify logging APIs
**Risk**  
Mixed `printf()` and `UART2_DMA_Log()` increases hidden dependency on stdio retarget.

**Where**
- `board.c`

**Action**
- Route all logs through DMA logger
- Or explicitly document `printf()` usage window

**Benefit**
- Cleaner abstraction
- Easier porting

---

### C2. Add dropped-log health reporting
**Risk**  
Dropped logs go unnoticed during stress.

**Where**
- `uart_dma.c`

**Action**
- Periodically emit:
    - “LOG DROPS: N”
- Or expose getter for health monitoring

**Benefit**
- Better observability under load

---

### C3. Replace magic GCLK channel numbers with named macros
**Risk**  
DFP changes can silently break code.

**Where**
- `cpu.c`

**Action**
- Use instance macros where available
- Document any unavoidable numeric IDs

**Benefit**
- Improves portability across SAME5x variants

---

## Architectural Notes (Non-Blocking Design Contract)

- Main loop is **cooperative**
- Only SysTick and DMAC interrupts are allowed
- Blocking APIs must be avoided in application paths
- DMA logging is **main-context only**

Violating these assumptions will degrade determinism.

---

## Final Reviewer Note

This project is **architecturally sound** and already above average for bare-metal firmware.  
The main risks are **clock-related hangs and hidden blocking paths**, not correctness bugs.

Fixing the P0 items will significantly improve **field robustness** without increasing complexity.

---
