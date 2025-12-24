#ifndef BOARD_H
#define BOARD_H

#include "sam.h"
#include <stdbool.h>
//#include "component/port.h"   // for PORT_PINCFG_INEN_Msk, PORT_PINCFG_PULLEN_Msk


//-----------------------------------------------------------------------------
// Firmware version
//-----------------------------------------------------------------------------
#ifndef FW_VERSION_H
#define FW_VERSION_H

#define FW_NAME        "SAME54_BareMetal"
#define FW_VERSION     "v1.0.0"

/* Optional semantic breakdown */
#define FW_VER_MAJOR   1
#define FW_VER_MINOR   0
#define FW_VER_PATCH   0

#endif
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
#define BOARD_DFLL48M_HZ        48000000UL

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
// RTCC
//-----------------------------------------------------------------------------
#define BOARD_ENABLE_RTCC
//-----------------------------------------------------------------------------
// QSPI - N25Q256A
//-----------------------------------------------------------------------------

/* ------------------------------------------------------------------------- */
/* QSPI memory mapping                                                     */
/*
 * On the SAM D5x/E5x family (including the SAME54), the external QSPI flash
 * appears in the AHB address space when the QSPI controller is configured
 * for serial memory mode.  CircuitPython’s port.c notes that the QSPI
 * region spans 16 MiB starting at 0x04000000【752595409825279†L576-L581】.
 * The memory map is therefore defined below for convenience.  If you
 * attach a different size device you should adjust QSPI_AHB_SIZE to match.
 */

/** Base address of the QSPI memory region on the SAME54. */
#define QSPI_AHB_BASE  ((uintptr_t)0x04000000U)

/** Maximum size of the QSPI address space (16 MiB). */
#define QSPI_AHB_SIZE  (16U * 1024U * 1024U)

/* ------------------------------------------------------------------------- */
/* Flash organisation                                                       */
/*
 * The Micron N25Q256A on the SAM E54 Xplained Pro contains 256 Mbit
 * (32 MiB) of storage arranged as 64kB erase sectors and 256‑byte program
 * pages.  These definitions are provided for convenience when allocating
 * storage regions or computing alignments.
 */

/** Sector (erase block) size in bytes. */
#define QSPI_FLASH_SECTOR_SIZE   (64U * 1024U)

/** Page (program) size in bytes. */
#define QSPI_FLASH_PAGE_SIZE     256U

/** Total capacity of the on‑board N25Q256A flash (32 MiB). */
#define QSPI_FLASH_CAPACITY      (32U * 1024U * 1024U)
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
typedef struct {
    sw_id_t              id;
    uint32_t     t_debounce;
    uint32_t            cnt;
} sw_t;
//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
void FW_LogBanner(void);
void board_init(void);
bool board_led0_is_on(void);
void board_led0_on(void);
void board_led0_off(void);
void board_led0_toggle(void);
bool board_sw_pressed(sw_t *sw);



#endif
