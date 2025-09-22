#ifndef BOARD_H
#define BOARD_H

#include "sam.h"
#include <stdbool.h>

//-----------------------------------------------------------------------------
// System Clock
//-----------------------------------------------------------------------------
#define BOARD_CPU_CLOCK       120000000UL
#define BOARD_XOSC0_FREQ      12000000UL

//-----------------------------------------------------------------------------
// Port group aliases (PIC32MZ TRIS/LAT style)
//-----------------------------------------------------------------------------
#define TRISAbits   (PORT->Group[0].DIR)
#define TRISBbits   (PORT->Group[1].DIR)
#define TRISCbits   (PORT->Group[2].DIR)

#define LATAbits    (PORT->Group[0].OUT)
#define LATBbits    (PORT->Group[1].OUT)
#define LATCbits    (PORT->Group[2].OUT)

#define LATASET     (PORT->Group[0].OUTSET)
#define LATACLR     (PORT->Group[0].OUTCLR)
#define LATATGL     (PORT->Group[0].OUTTGL)

#define LATBSET     (PORT->Group[1].OUTSET)
#define LATBCLR     (PORT->Group[1].OUTCLR)
#define LATBTGL     (PORT->Group[1].OUTTGL)

#define LATCSET     (PORT->Group[2].OUTSET)
#define LATCCLR     (PORT->Group[2].OUTCLR)
#define LATCTGL     (PORT->Group[2].OUTTGL)

#define PORTAINbits (PORT->Group[0].IN)
#define PORTBINbits (PORT->Group[1].IN)
#define PORTCINbits (PORT->Group[2].IN)

//-----------------------------------------------------------------------------
// LED0 (PC18, active low)
//-----------------------------------------------------------------------------
#define LED0_PORT     2
#define LED0_PIN      18
#define LED0_MASK     (1 << LED0_PIN)

// Inline macros for LED0 control
#define LED0_On()       (LATCCLR = LED0_MASK)  // drive low → ON
#define LED0_Off()      (LATCSET = LED0_MASK)  // drive high → OFF
#define LED0_Toggle()   (LATCTGL = LED0_MASK)

//-----------------------------------------------------------------------------
// Button SW0 (PB31, active low, needs pull-up)
//-----------------------------------------------------------------------------
#define BUTTON0_PORT  1
#define BUTTON0_PIN   31
#define BUTTON0_MASK  (1 << BUTTON0_PIN)

// Inline macro for SW0 read
#define SW0_Pressed()   ((PORTBINbits.reg & BUTTON0_MASK) == 0)  // active low

//-----------------------------------------------------------------------------
// UART (SERCOM2, PB24 RX, PB25 TX)
//-----------------------------------------------------------------------------
#define BOARD_UART_BAUDRATE   115200UL
#define BOARD_UART_DMA_CH     0

#define UART_SERCOM_INDEX     2
#define UART_TX_PORT          1
#define UART_TX_PIN           25
#define UART_TX_PMUX          MUX_PB25C_SERCOM2_PAD0
#define UART_RX_PORT          1
#define UART_RX_PIN           24
#define UART_RX_PMUX          MUX_PB24C_SERCOM2_PAD1

//-----------------------------------------------------------------------------
// SWO (PB30)
//-----------------------------------------------------------------------------
#define BOARD_SWO_BAUDRATE    2000000UL
#define SWO_PORT              1
#define SWO_PIN               30

//-----------------------------------------------------------------------------
// Timing
//-----------------------------------------------------------------------------
#define BOARD_LED_BLINK_MS    1000

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
void SystemConfigPerformance(void);
void BOARD_Init(void);


#endif
