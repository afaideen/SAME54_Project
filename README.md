# SAME54 Project https://github.com/afaideen/SAME54_Project

Bare-metal firmware for the **Microchip SAME54 Xplained Pro** development board.  
The project uses a **PIC32MZ-style coding approach** (TRIS/LAT/PORT macros) for GPIO handling, while keeping full CMSIS register access for clock and peripheral setup.  

---

## 🔹 Features
- **System Clock Setup**  
  - Configures XOSC0 (12 MHz) → DPLL0 → CPU at 120 MHz  
  - Flash wait states + cache enabled  

- **GPIO**  
  - **LED0 (PC18, active low)** with `LED0_On()`, `LED0_Off()`, `LED0_Toggle()` macros  
  - **SW0 (PB31, active low)** with `SW0_Pressed()` macro  

- **UART2 (VCOM)**  
  - TX = PB25, RX = PB24  
  - DMA support with callback  
  - `printf()` retargeting  

- **SWO Debug Output**  
  - PB30 at 2 Mbps  

- **SysTick + Delay**  
  - 1 ms tick  
  - `DelayMs()` non-blocking time handling (wraparound-safe, up to ~49 days)  

---

## 🔹 Repository Layout
```
/common
  ├── board.h / board.c   # Hardware profile + init
  ├── systick.*           # SysTick driver
  ├── delay.*             # Delay helper
/drivers
  ├── uart.*              # UART2 + DMA driver
  ├── swo.*               # SWO debug output
main.c                    # Application entry point
```

---

## 🔹 Quick Start
1. Clone the repo:
   ```bash
   git clone https://github.com/afaideen/SAME54_Project.git
   cd SAME54_Project
   ```
2. Build with your toolchain (e.g. ARM GCC or Atmel Studio).  
3. Flash to the **SAME54 Xplained Pro** via EDBG USB.  
4. Open a serial terminal at **115200 8N1** → see `printf()` and DMA-driven messages.  
5. Press **SW0** → check LED0 and debug output.  

---

## 🔹 Hardware Reference
- **LED0:** PC18 (active low)  
- **SW0:** PB31 (active low, enable internal pull-up)  
- **UART2:** PB24 (RX), PB25 (TX)  
- **SWO:** PB30  

For more details, see the included  
📄 [`SAME54_Xplained_CheatSheet.md`](./SAME54_Xplained_CheatSheet.md)  
