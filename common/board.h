#ifndef BOARD_H
#define BOARD_H

#include "sam.h"
#include <stdbool.h>
//#include "component/port.h"   // for PORT_PINCFG_INEN_Msk, PORT_PINCFG_PULLEN_Msk


//-----------------------------------------------------------------------------
// System Clock 
//-----------------------------------------------------------------------------
/* Board crystal + DPLL0 target */
#define BOARD_XOSC0_FREQ      12000000UL
#define BOARD_DPLL0_FREQ      120000000UL

/* === Only change this === */
#define BOARD_CPU_CLOCK       120000000UL   /* or 120000000UL */
//#define BOARD_CPU_CLOCK        60000000UL   /* or  60000000UL */

#define CPU_CLOCK_HZ          BOARD_CPU_CLOCK

/* CPU divider from DPLL0 */
#define CPU_DIVIDER           (BOARD_DPLL0_FREQ / BOARD_CPU_CLOCK)

#if (BOARD_DPLL0_FREQ % BOARD_CPU_CLOCK) != 0
#error "BOARD_CPU_CLOCK must divide BOARD_DPLL0_FREQ evenly"
#endif

#if (CPU_DIVIDER < 1) || (CPU_DIVIDER > 255)
#error "CPU_DIVIDER out of range for MCLK_CPUDIV"
#endif



/* GCLK0 is DPLL0 / 1 */
#define GCLK0_DIVIDER         1u
#define GCLK0_CLOCK_HZ        (BOARD_DPLL0_FREQ / GCLK0_DIVIDER)

/* Make GCLK1 follow CPU rate: DPLL0 / CPU_DIVIDER */
#define GCLK1_DIVIDER         (CPU_DIVIDER)
#define GCLK1_CLOCK_HZ        (BOARD_DPLL0_FREQ / GCLK1_DIVIDER)

#if (GCLK1_CLOCK_HZ != CPU_CLOCK_HZ)
#error "UART expects GCLK1 to match CPU clock"
#endif

/*-----------------------------------------------------------------------------
// UART SERCOM2
//-----------------------------------------------------------------------------
/* SAME54 Xplained Pro EDBG VCOM UART:
   PB24 = SERCOM2 PAD1 (RX)
   PB25 = SERCOM2 PAD0 (TX)
   Peripheral function = D
*/
#define UART_RX_PORT_GROUP     (1U)    /* 0=A, 1=B, 2=C, 3=D */
#define UART_RX_PIN            (24U)   /* PB24 */
#define UART_TX_PORT_GROUP     (1U)    /* PORTB */
#define UART_TX_PIN            (25U)   /* PB25 */
#define UART_PMUX_FUNC_D       (PORT_PMUX_PMUXE_D_Val) /* same numeric value used for PMUXE/PMUXO */
#define UART_BAUDRATE           115200u
//-----------------------------------------------------------------------------
// PORT helper macros 
//-----------------------------------------------------------------------------
#define PORT_DIRSET(port, mask)   (PORT_REGS->GROUP[(port)].PORT_DIRSET = (mask))
#define PORT_DIRCLR(port, mask)   (PORT_REGS->GROUP[(port)].PORT_DIRCLR = (mask))
#define PORT_OUTSET(port, mask)   (PORT_REGS->GROUP[(port)].PORT_OUTSET = (mask))
#define PORT_OUTCLR(port, mask)   (PORT_REGS->GROUP[(port)].PORT_OUTCLR = (mask))
#define PORT_OUTTGL(port, mask)   (PORT_REGS->GROUP[(port)].PORT_OUTTGL = (mask))
#define PORT_IN(port)             (PORT_REGS->GROUP[(port)].PORT_IN)

#define PORT_PINCFG_SET(port, pin, mask)    (PORT_REGS->GROUP[(port)].PORT_PINCFG[(pin)] = (uint8_t)(mask))

#define PORT_PINCFG_OR(port, pin, mask)     (PORT_REGS->GROUP[(port)].PORT_PINCFG[(pin)] |= (uint8_t)(mask))

#define PORT_PINCFG_INEN_PULLEN             (PORT_PINCFG_INEN_Msk | PORT_PINCFG_PULLEN_Msk)

//-----------------------------------------------------------------------------
// LED0 (PC18, active low)
//-----------------------------------------------------------------------------
#define LED0_PORT     2u
#define LED0_PIN      18u
#define LED0_MASK     (1u << LED0_PIN)

#define LED0_On()       PORT_OUTCLR(LED0_PORT, LED0_MASK)  // low => ON
#define LED0_Off()      PORT_OUTSET(LED0_PORT, LED0_MASK)  // high => OFF
#define LED0_Toggle()   PORT_OUTTGL(LED0_PORT, LED0_MASK)

//-----------------------------------------------------------------------------
// Button SW0 (PB31, active low, needs pull-up)
//-----------------------------------------------------------------------------
#define BUTTON0_PORT  1u
#define BUTTON0_PIN   31u
#define BUTTON0_MASK  (1u << BUTTON0_PIN)

#define SW0_Pressed()  ((PORT_IN(BUTTON0_PORT) & BUTTON0_MASK) == 0u)
#define DEBOUNCE_TIME       200

//-----------------------------------------------------------------------------
// Variable type
//-----------------------------------------------------------------------------
typedef enum {
    SW0 = 0,
    /* future: SW1, SW2 */
} sw_id_t;
typedef struct {
    bool     stable_state;
    bool     last_raw;
    uint32_t press_start_ms;
} sw_state_t;
//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

void BOARD_Init(void);
bool led0_is_on(void);
bool sw_pressed(sw_id_t sw, uint32_t debounce);



#endif
