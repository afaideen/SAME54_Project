# HARDWARE_PROFILE_CHECKS.md — SAME54 Xplained Pro bring-up checks (for this project)

This checklist verifies the **exact hardware assumptions** used by the firmware: GPIO polarity, SysTick timing, SERCOM2 UART logging over EDBG VCOM, and DMAC-driven TX behavior at 60 MHz and 120 MHz.

---

## 1. Pre-flight setup

### 1.1 Tools
- MPLAB X + XC32 (pic32c) toolchain
- USB cable to SAM E54 Xplained Pro (EDBG)
- Serial terminal (Tera Term / PuTTY / MobaXterm)
- Optional: oscilloscope or logic analyzer

### 1.2 Confirm board target
- Device: ATSAME54P20A
- Board: SAM E54 Xplained Pro
- EDBG provides:
    - programming/debug
    - Virtual COM Port (VCOM)

---

## 2. GPIO sanity checks (LED0 and SW0)

### 2.1 LED0 polarity check (PC18 active-low)
**Expected**
- LED0 ON when PC18 output is **0**
- LED0 OFF when PC18 output is **1**

**How to verify**
1. Flash firmware.
2. Observe LED0 blink.
3. Optional probe:
    - Put scope probe on PC18:
        - LED ON should coincide with PC18 near 0V
        - LED OFF should coincide with PC18 near 3.3V

**Pass criteria**
- Visible blink at expected rate (default 500 ms period)
- PC18 toggles high/low accordingly

---

### 2.2 SW0 pull-up and active-low check (PB31)
**Expected**
- PB31 reads **1** when not pressed
- PB31 reads **0** when pressed
- Internal pull-up is enabled in firmware

**How to verify**
1. Run firmware.
2. Press SW0 and hold:
    - You should see a “pressed” log event (after debounce threshold).
    - Blink period should change as defined by your app logic.

**Optional probe**
- Probe PB31:
    - idle should sit near 3.3V
    - pressed should drop close to 0V

**Pass criteria**
- Idle high, pressed low behavior matches expectations
- Debounce prevents false multiple triggers on a single press

---

## 3. SysTick timebase checks (1 ms tick)

### 3.1 `millis()` monotonicity
**Expected**
- `millis()` increments at ~1000 counts per second.

**How to verify**
1. Add a temporary log that prints `millis()` once per second using `DelayMsAsync`.
2. Compare observed increments:
    - should be ~1000 per second.

**Pass criteria**
- `millis()` increases monotonically
- drift is acceptable for 60/120 MHz configs

---

## 4. UART over EDBG VCOM checks (SERCOM2)

### 4.1 Identify correct COM port
**Steps**
1. Connect board via USB.
2. Open Device Manager (Windows) → Ports.
3. Look for EDBG Virtual COM.

**Pass criteria**
- COM port appears and is selectable in terminal program

### 4.2 Baud and output integrity
**Expected**
- Clean ASCII logs (no garbage characters)
- Consistent line endings

**Steps**
1. Open terminal with firmware’s configured baud (your project’s default).
2. Reset board.
3. Observe boot banner and periodic LED logs.

**Pass criteria**
- No repeated junk characters
- No missing lines under normal logging load

---

## 5. DMAC-driven UART TX checks (non-blocking logger)

### 5.1 “Non-blocking” behavior under load
**Goal**
Confirm UART logging uses DMAC and does not stall the main loop.

**Steps**
1. Increase log rate temporarily (e.g., log every 10 ms).
2. Confirm LED blink logic still runs correctly (period correct).
3. Confirm logs continue without freezing the app.

**Pass criteria**
- LED blink stays accurate while logs stream
- System remains responsive to SW0 press events

### 5.2 Queue overflow behavior
**Goal**
Confirm queue-full policy drops logs rather than blocking.

**Steps**
1. Spam logs faster than UART throughput.
2. Observe:
    - logs may become discontinuous (dropped)
    - app still runs normally

**Pass criteria**
- No system lockups
- Dropped logs acceptable and expected

### 5.3 Trigger pacing verification (optional scope)
**Goal**
Confirm UART TX is peripheral-paced by SERCOM2 trigger.

**Steps**
1. Probe PB25 (TX) and capture bursts.
2. Observe continuous UART waveform during log output.
3. Confirm no long idle stalls between bytes when queue has data.

**Pass criteria**
- UART frames are continuous when queue is non-empty
- TX waveform matches expected baud timing

---

## 6. RTCC timestamp checks (optional calendar time)

### 6.1 Boot-time behavior
**Expected**
- If RTCC is valid, log prefix includes date time.
- If not valid, prefix uses placeholder.

**Steps**
1. Cold boot board.
2. Observe first log lines:
    - should show either real date time or placeholder depending on RTCC validity and sync logic.

**Pass criteria**
- RTCC init does not hang
- Prefix format is stable

### 6.2 Crystal readiness (important)
If using XOSC32K-based 1K source:
- firmware may wait for oscillator-ready.
- on a variant board without crystal startup, this can hang.

**Pass criteria**
- On Xplained Pro board, RTCC init completes.
- For crystal-less variants, switch to internal ULP1K option or add timeouts.

---

## 7. Dual frequency validation (60 MHz and 120 MHz)

### 7.1 Test matrix

| Config | Expected LED period | Expected UART output |
|---|---:|---|
| CPU 120 MHz | correct 500 ms | clean logs |
| CPU 60 MHz | correct 500 ms | clean logs |

**Steps**
1. Build with 120 MHz config, flash and verify:
    - blink period, UART clean, timestamps correct
2. Build with 60 MHz config, flash and verify:
    - blink period still correct
    - UART still clean (no garbage)

**Pass criteria**
- Both configs behave identically at the application level

---

## 8. Common failure signatures and what they usually mean

- **Garbage UART characters**
    - SERCOM clock source or divider mismatch
    - baud calc assumes GCLK1 equals CPU clock but it doesn’t
    - wrong terminal baud

- **No UART output**
    - wrong COM port
    - SERCOM2 not initialized
    - DMAC not enabled and blocking path not used

- **LED blink wrong speed**
    - SysTick initialized with wrong CPU clock
    - CPU clock divider not applied as expected

- **System hangs at boot**
    - waiting forever for oscillator-ready (RTCC crystal path)
    - stuck SYNCBUSY wait in clock or RTC init

---

## 9. Recommended minimal test after each change

1. Boot banner prints once.
2. LED toggles at 500 ms for at least 10 seconds.
3. Press SW0:
    - one qualified press triggers one mode change
4. Switch CPU config 60 ↔ 120:
    - repeat steps 1–3.

---
