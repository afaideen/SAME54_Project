//
// Created by han on 22-Dec-25.
//
#include <stdio.h>
#include <sam.h>
#include "board.h"
#include "cpu.h"
#include "../drivers/uart.h"
#include "../drivers/uart_dma.h"

// ****************************************************************************
// ****************************************************************************
// Section: Configuration Bits
// ****************************************************************************
// ****************************************************************************

#pragma config BOD33_DIS = SET
#pragma config BOD33USERLEVEL = 0x1c
#pragma config BOD33_ACTION = RESET
#pragma config BOD33_HYST = 0x2
#pragma config NVMCTRL_BOOTPROT = 0
#pragma config NVMCTRL_SEESBLK = 0x0
#pragma config NVMCTRL_SEEPSZ = 0x0
#pragma config RAMECC_ECCDIS = SET
#pragma config WDT_ENABLE = CLEAR
#pragma config WDT_ALWAYSON = CLEAR
#pragma config WDT_PER = CYC8192
#pragma config WDT_WINDOW = CYC8192
#pragma config WDT_EWOFFSET = CYC8192
#pragma config WDT_WEN = CLEAR
#pragma config NVMCTRL_REGION_LOCKS = 0xffffffff

/* Optional: header-model guard */
#if !defined(MCLK_REGS) || !defined(GCLK_REGS) || !defined(OSCCTRL_REGS) || !defined(OSC32KCTRL_REGS) || !defined(NVMCTRL_REGS) || !defined(CMCC_REGS)
#error "Header mismatch: expected SAME54 DFP with *_REGS pointers"
#endif

static inline void RTCC_ForceOffAtBoot(void)
{
    // Optional but recommended: stop IRQ side effects
    NVIC_DisableIRQ(RTC_IRQn);

    // Disable RTC (MODE2 view)
    RTC_REGS->MODE2.RTC_CTRLA &= (uint16_t)~RTC_MODE2_CTRLA_ENABLE_Msk;
    while ((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_ENABLE_Msk) != 0U) { }

    // Clear/disable interrupts (covers previous configs)
    RTC_REGS->MODE2.RTC_INTENCLR = RTC_MODE2_INTENCLR_Msk;
    RTC_REGS->MODE2.RTC_INTFLAG  = RTC_MODE2_INTFLAG_Msk;

    // Optional: hard reset RTC registers to defaults (strongest ?OFF?)
    RTC_REGS->MODE2.RTC_CTRLA = RTC_MODE2_CTRLA_SWRST_Msk;
    while ((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_SWRST_Msk) != 0U) { }
}


static void OSCCTRL_Initialize_bm(void)
{
    /* plib_clock.c leaves this empty */
}

static void OSC32KCTRL_Initialize_bm(void)
{
    /* Direct copy from plib_clock.c */
    OSC32KCTRL_REGS->OSC32KCTRL_RTCCTRL = OSC32KCTRL_RTCCTRL_RTCSEL(0);
}

static void DFLL_Initialize_bm(void)
{
    /* plib_clock.c leaves this empty */
}

static void GCLK2_Initialize_bm(void)
{
    /* Direct copy from plib_clock.c */
    GCLK_REGS->GCLK_GENCTRL[2] =
        GCLK_GENCTRL_DIV(48) |
        GCLK_GENCTRL_SRC(6) |
        GCLK_GENCTRL_GENEN_Msk;

    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK2) == GCLK_SYNCBUSY_GENCTRL_GCLK2)
    {
        /* wait for Generator 2 synchronization */
    }
}

static void FDPLL0_Initialize_bm(void)
{
    /* Direct copy from plib_clock.c */

    /* Route GCLK2 to DPLL0 reference channel */
    GCLK_REGS->GCLK_PCHCTRL[1] = GCLK_PCHCTRL_GEN(0x2) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[1] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        /* Wait for synchronization */
    }

    /****************** DPLL0 Initialization *********************************/

    /* Configure DPLL */
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLB =
        OSCCTRL_DPLLCTRLB_FILTER(0) |
        OSCCTRL_DPLLCTRLB_LTIME(0x0) |
        OSCCTRL_DPLLCTRLB_REFCLK(0);

    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLRATIO =
        OSCCTRL_DPLLRATIO_LDRFRAC(0) |
        OSCCTRL_DPLLRATIO_LDR(119);

    while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY & OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk) ==
           OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk)
    {
        /* Waiting for the synchronization */
    }

    /* Enable DPLL */
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLA = OSCCTRL_DPLLCTRLA_ENABLE_Msk;

    while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY & OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk) ==
           OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk)
    {
        /* Waiting for the DPLL enable synchronization */
    }

    while ((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSTATUS &
            (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk)) !=
           (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk))
    {
        /* Waiting for the Ready state */
    }
}

static void GCLK0_Initialize_bm(void)
{
    /*
     * Configure the CPU and generic clock generator 0.
     *
     * Previously this function hard‑coded the CPU and GCLK0 divisors to 1.
     * To make the code more flexible and reduce duplication in
     * SystemConfigPerformance(), this implementation uses the
     * CPU_DIVIDER and GCLK0_DIVIDER macros. These constants are
     * typically defined in a board or CPU header and allow the
     * application to select an appropriate division ratio for the
     * processor clock and the generator driving it.
     */

    /* Select CPU clock division.  The CPU_DIVIDER macro is expected to
     * produce the desired division factor for the core clock.  Casting
     * to uint8_t prevents any sign/size promotion warnings.
     */
    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV((uint8_t)CPU_DIVIDER);

    /* Wait for the main clock to become ready.  The CKRDY flag is
     * asserted once the clock division has taken effect.
     */
    while ((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) != MCLK_INTFLAG_CKRDY_Msk)
    {
        /* Busy‑wait for synchronization */
    }

    /* Configure generic clock generator 0.  The GCLK0_DIVIDER macro
     * specifies the division factor applied to the DPLL0 source for
     * peripheral clocks, while the source is fixed to DPLL0 (SRC=7).
     * Generator 0 drives the CPU and other performance‑sensitive
     * peripherals.
     */
    GCLK_REGS->GCLK_GENCTRL[0] =
        GCLK_GENCTRL_DIV(GCLK0_DIVIDER) |
        GCLK_GENCTRL_SRC(7) |
        GCLK_GENCTRL_GENEN_Msk;

    /* Wait for generator 0 to synchronize after enabling.  The
     * SYNCBUSY flag clears once the generator is fully configured.
     */
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK0) == GCLK_SYNCBUSY_GENCTRL_GCLK0)
    {
        /* Busy‑wait for synchronization */
    }
}

static void GCLK1_Initialize_bm(void)
{
    /*
     * Configure generic clock generator 1 using the application‑defined
     * divider.  Generator 1 is typically used for peripheral clocks
     * such as UARTs.  To avoid hard‑coded values in the system
     * initialization function, this implementation uses the
     * GCLK1_DIVIDER macro.  The source is set to DPLL0 (SRC=7).
     */
    GCLK_REGS->GCLK_GENCTRL[1] =
        GCLK_GENCTRL_DIV(GCLK1_DIVIDER) |
        GCLK_GENCTRL_SRC(7) |
        GCLK_GENCTRL_GENEN_Msk;

    /* Wait for generator 1 to synchronize after enabling. */
    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK1) == GCLK_SYNCBUSY_GENCTRL_GCLK1)
    {
        /* Busy‑wait for synchronization */
    }
}

/**
 * @brief Configure flash wait states and enable the instruction cache.
 *
 * The system must increase the number of flash wait states when running
 * the CPU at higher frequencies.  In addition, enabling the cache
 * improves instruction throughput.  This helper groups those
 * operations together and ensures that the relevant buses are
 * temporarily unmasked while the NVM and CMCC registers are modified.
 */
//static void FlashAndCache_Initialize_bm(void)
//{
//    /* Ensure NVMCTRL and CMCC are clocked while we adjust them.  Also
//     * enable clocking to GCLK, OSCCTRL and OSC32KCTRL since some
//     * downstream initializers may touch them.  These bits are ORed
//     * because other code may set additional mask bits elsewhere.
//     */
//    MCLK_REGS->MCLK_AHBMASK  |= (MCLK_AHBMASK_NVMCTRL_Msk | MCLK_AHBMASK_CMCC_Msk);
//    MCLK_REGS->MCLK_APBAMASK |= (MCLK_APBAMASK_GCLK_Msk |
//                                 MCLK_APBAMASK_OSCCTRL_Msk |
//                                 MCLK_APBAMASK_OSC32KCTRL_Msk);
//
//    /* Configure flash wait states.  The SAME54 requires five wait
//     * states for correct operation at 120 MHz.  The RWS field lives
//     * in CTRLA, not CTRLB.  Clear any previous value then OR in the
//     * new setting.
//     */
//    NVMCTRL_REGS->NVMCTRL_CTRLA = (NVMCTRL_REGS->NVMCTRL_CTRLA & (uint16_t)~NVMCTRL_CTRLA_RWS_Msk) |
//                        (uint16_t)(NVMCTRL_CTRLA_RWS(5) | NVMCTRL_CTRLA_AUTOWS_Msk);
//
//
//    /* Enable the instruction cache.  Without this the CPU would stall
//     * on every flash access once the branch predictor runs out of
//     * entries.  The CMCC peripheral exposes a single control register
//     * with a cache enable bit.
//     */
//    CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;
//}
static void FlashAndCache_Initialize_bm(void)
{
    /* Flash wait states config stays as-is */

    /* --- Disable CMCC cache for QSPI correctness test (Harmony-like) --- */
    CMCC_REGS->CMCC_CTRL &= ~CMCC_CTRL_CEN_Msk;
    __DSB();
    __ISB();

    /* Do NOT enable cache here for now */
    // CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;
}

/**
 * @brief Enable peripheral generic clock channels.
 *
 * This helper sets up the GCLK peripheral channels required by the
 * application.  It configures channels for the External Interrupt
 * Controller (EIC) and for the SERCOM2/SERCOM3 cores.  Each channel
 * uses generator 1 as its clock source, which is configured by
 * GCLK1_Initialize_bm().
 */
static void PeripheralClocks_Initialize_bm(void)
{
    /* External Interrupt Controller clock: channel 4 */
    GCLK_REGS->GCLK_PCHCTRL[4] = GCLK_PCHCTRL_GEN(0x1) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[4] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        /* Wait for synchronization */
    }

    /* SERCOM2 Core clock: channel 23 */
    GCLK_REGS->GCLK_PCHCTRL[23] = GCLK_PCHCTRL_GEN(0x1) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[23] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        /* Wait for synchronization */
    }

    /* SERCOM3 Core clock: channel 24 */
    GCLK_REGS->GCLK_PCHCTRL[24] = GCLK_PCHCTRL_GEN(0x1) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[24] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        /* Wait for synchronization */
    }
}

/**
 * @brief Configure the MCLK bus masks to match the CLOCK_Initialize()
 *        defaults from Harmony.
 *
 * After all clocks are configured, Harmony's CLOCK_Initialize()
 * function sets specific bus mask values.  Consolidating these writes
 * into a helper improves readability in the system initialization.
 */
static void MCLK_Masks_Initialize_bm(void)
{
    MCLK_REGS->MCLK_AHBMASK  = 0x00FFFFFFu;
    MCLK_REGS->MCLK_APBAMASK = 0x000007FFu;
    MCLK_REGS->MCLK_APBBMASK = 0x00018656u;
}

/**
 * @brief Enable the ARM DWT cycle counter for microsecond timing.
 *
 * The Data Watchpoint and Trace (DWT) unit provides a cycle counter
 * that can be used to measure short delays without resorting to
 * software loops.  This helper enables the counter and clears it.
 */
static void EnableDwtCycleCounter_bm(void)
{
    /* Enable the trace and debug blocks in the ARM core */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Reset the cycle counter to zero */
    DWT->CYCCNT = 0;
    /* Enable the cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * Configures CPU for optimal performance
 * 
 * Initializes:
 * - Flash wait states (RWS=5)
 * - Instruction cache (CMCC)
 * - DPLL0 clock generator (120 MHz)
 * - Generic clock generators (GCLK0, GCLK1, GCLK2)
 * - CPU and peripheral clock dividers
 * - Peripheral clock channels (EIC, SERCOM2, SERCOM3)
 * - MCLK bus masks
 * 
 * Should be called early in startup before using any peripherals
 */
void SystemConfigPerformance(void)
{
    /*
     * The original implementation of SystemConfigPerformance() contained
     * a large amount of clock configuration code duplicated from
     * Harmony’s plib_clock.c.  To make the code more maintainable and
     * readable, this function has been refactored to call a series of
     * helper functions.  Each helper encapsulates a specific aspect
     * of the initialization sequence.
     */

    /* A) Performance configuration: flash wait states and cache enable. */
    FlashAndCache_Initialize_bm();

    /* B) Clock tree initialization.  These functions configure the
     *    oscillator and generic clock generators exactly once.  They
     *    replace the inlined copies that previously lived here.
     */
    OSCCTRL_Initialize_bm();     /* Empty, provided for completeness */
    OSC32KCTRL_Initialize_bm();  /* Configure RTC clock selection */
    DFLL_Initialize_bm();        /* Empty, provided for completeness */
    GCLK2_Initialize_bm();       /* Configure generator 2: SRC(6)/48 */
    FDPLL0_Initialize_bm();      /* Route GEN2 to DPLL0 and lock at 120 MHz */
    GCLK0_Initialize_bm();       /* Configure CPU clock using CPU_DIVIDER/GCLK0_DIVIDER */
    GCLK1_Initialize_bm();       /* Configure peripheral generator 1 using GCLK1_DIVIDER */

    /* Peripheral clock channels (EIC, SERCOM2/3) */
    PeripheralClocks_Initialize_bm();

    /* MCLK bus masks */
    MCLK_Masks_Initialize_bm();

    /* Enable the DWT cycle counter for microsecond resolution timing */
    EnableDwtCycleCounter_bm();

    /* Ensure the RTC is fully disabled at boot.  This call stops the
     * counter, clears interrupts and performs a software reset on the
     * MODE2 register block.
     */
    RTCC_ForceOffAtBoot();
}



void CPU_LogClockOverview(void)
{
    /* ---------- Read hardware state ---------- */

    uint32_t gclk0_src =
        (GCLK_REGS->GCLK_GENCTRL[0] & GCLK_GENCTRL_SRC_Msk)
        >> GCLK_GENCTRL_SRC_Pos;

    const char *gclk0_src_str =
        (gclk0_src == 7) ? "DPLL0" :
        (gclk0_src == 6) ? "DFLL"  : "UNKNOWN";

    uint32_t ldr =
        (OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLRATIO &
         OSCCTRL_DPLLRATIO_LDR_Msk) >>
         OSCCTRL_DPLLRATIO_LDR_Pos;

    uint32_t ldrfrac =
        (OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLRATIO &
         OSCCTRL_DPLLRATIO_LDRFRAC_Msk) >>
         OSCCTRL_DPLLRATIO_LDRFRAC_Pos;
    
    /* --- Derive DPLL0 reference from GCLK2 --- */

uint32_t gclk2_src =
    (GCLK_REGS->GCLK_GENCTRL[2] & GCLK_GENCTRL_SRC_Msk)
    >> GCLK_GENCTRL_SRC_Pos;

uint32_t gclk2_div =
    (GCLK_REGS->GCLK_GENCTRL[2] & GCLK_GENCTRL_DIV_Msk)
    >> GCLK_GENCTRL_DIV_Pos;

if (gclk2_div == 0)
    gclk2_div = 1;

/* Assume DFLL48M if SRC=6 (which matches your config) */
double gclk2_hz = 0.0;

if (gclk2_src == 6)  /* DFLL48M */
{
    gclk2_hz = (double)BOARD_DFLL48M_HZ / gclk2_div;
}

    double dpll0_hz = gclk2_hz * (ldr + 1.0 + (ldrfrac / 16.0));

    uint32_t gclk0_div =
        (GCLK_REGS->GCLK_GENCTRL[0] & GCLK_GENCTRL_DIV_Msk)
        >> GCLK_GENCTRL_DIV_Pos;
    if (gclk0_div == 0) gclk0_div = 1;

    uint32_t cpu_div =
        (MCLK_REGS->MCLK_CPUDIV & MCLK_CPUDIV_DIV_Msk)
        >> MCLK_CPUDIV_DIV_Pos;
    if (cpu_div == 0) cpu_div = 1;

    double cpu_hz = (dpll0_hz / gclk0_div) / cpu_div;

    uint32_t gclk1_div =
        (GCLK_REGS->GCLK_GENCTRL[1] & GCLK_GENCTRL_DIV_Msk)
        >> GCLK_GENCTRL_DIV_Pos;
    if (gclk1_div == 0) gclk1_div = 1;

    double gclk1_hz = dpll0_hz / gclk1_div;

    uint32_t systick_reload = SysTick->LOAD + 1;
    double systick_ms = (systick_reload * 1000.0) / cpu_hz;

    int rtcc_enabled =
        (RTC_REGS->MODE2.RTC_CTRLA & RTC_MODE2_CTRLA_ENABLE_Msk) ? 1 : 0;

    /* ---------- PRINTF banner ---------- */

    printf("\r\n================ CLOCK OVERVIEW ================\r\n");
    printf("CPU Clock      : %.3f MHz\r\n", cpu_hz / 1e6);
    printf("CPU Source     : %s\r\n", gclk0_src_str);
    printf("DPLL0 Output   : %.3f MHz\r\n", dpll0_hz / 1e6);
    printf("DPLL0 Ref      : %.3f MHz (GCLK2 = DFLL48M / %lu)\r\n", gclk2_hz / 1e6, gclk2_div);
    printf("DPLL0 Ratio    : LDR=%lu, LDRFRAC=%lu\r\n", ldr, ldrfrac);
    printf("GCLK0 Divider  : %lu\r\n", gclk0_div);
    printf("CPU Divider    : %lu\r\n", cpu_div);
    printf("GCLK1 (Periph) : %.3f MHz (DIV=%lu)\r\n", gclk1_hz / 1e6, gclk1_div);
    printf("SysTick        : %.3f ms tick\r\n", systick_ms);
    printf("DWT CYCCNT     : %s\r\n",
           (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) ? "ENABLED" : "DISABLED");
    printf("RTCC           : %s\r\n",
           rtcc_enabled ? "ENABLED" : "DISABLED");
    printf("===============================================\r\n\r\n");
}


