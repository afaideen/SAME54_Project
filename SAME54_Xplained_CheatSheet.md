# SAME54 Xplained Pro – Master Cheat Sheet

## 🖧 Debug / Communication
- **EDBG USB (Debug USB):**
  - VCOM UART: **SERCOM2 PB24 (RX), PB25 (TX)**
  - SWD: **SWCLK, SWDIO**
  - SWO: **PB30**

---

## 💡 User Peripherals
- **LED0:** **PC18**, active low  
- **SW0 (Button):** **PB31**, active low (requires internal pull-up)  

---

## 🔌 EXT1 / EXT2 / EXT3 Headers
- **SPI:** Shared with EXT1/2/3  
- **I²C:** Shared with EXT1/2/3  
- **USART:** Shared with EXT1/2/3  
- **GPIOs:** Various  

---

## 🚦 Other Key Peripherals
- **CAN:** PD12 (CAN TX), PD13 (CAN RX)  
- **Ethernet:** Multiple dedicated pins (RMII)  
- **QSPI Flash:** Dedicated QSPI pins  

---

## 🕒 System Clocks
- **XOSC0:** 12 MHz external crystal  
- **CPU Clock (DPLL0):** Up to 120 MHz  
