#include "board.h"
#include "systick.h"

void SystemConfigPerformance(void)
{
    /* --------------------------------------------------------------------
     * A) Performance bits that CMSIS/Harmony effectively ensures:
     *    - Flash wait states (your pack: RWS is in NVMCTRL_CTRLA)
     *    - Cache enable (CMCC)
     * -------------------------------------------------------------------- */

    /* Make sure these buses are clocked while we touch these peripherals
       (safe even if later we overwrite masks to match CLOCK_Initialize). */
    MCLK_REGS->MCLK_AHBMASK  |= (MCLK_AHBMASK_NVMCTRL_Msk | MCLK_AHBMASK_CMCC_Msk);
    MCLK_REGS->MCLK_APBAMASK |= (MCLK_APBAMASK_GCLK_Msk | MCLK_APBAMASK_OSCCTRL_Msk | MCLK_APBAMASK_OSC32KCTRL_Msk);

    /* Flash wait states: RWS(5) in CTRLA (NOT CTRLB in your pack) */
    NVMCTRL_REGS->NVMCTRL_CTRLA =
        (NVMCTRL_REGS->NVMCTRL_CTRLA & (uint16_t)~NVMCTRL_CTRLA_RWS_Msk) |
        (uint16_t)NVMCTRL_CTRLA_RWS(5);

    /* Cache enable */
    CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;

    /* --------------------------------------------------------------------
     * B) Direct inline translation of plib_clock.c -> CLOCK_Initialize()
     * -------------------------------------------------------------------- */

    /* OSCCTRL_Initialize() is empty in your plib_clock.c */

    /* OSC32KCTRL_Initialize(): */
    OSC32KCTRL_REGS->OSC32KCTRL_RTCCTRL = OSC32KCTRL_RTCCTRL_RTCSEL(0);

    /* DFLL_Initialize() is empty in your plib_clock.c */

    /* GCLK2_Initialize(): GEN2 = SRC(6) / 48 */
    GCLK_REGS->GCLK_GENCTRL[2] =
        GCLK_GENCTRL_DIV(48) |
        GCLK_GENCTRL_SRC(6) |
        GCLK_GENCTRL_GENEN_Msk;

    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK2) == GCLK_SYNCBUSY_GENCTRL_GCLK2)
    {
        /* wait for Generator 2 synchronization */
    }

    /* FDPLL0_Initialize(): Route GEN2 to DPLL0 ref (PCHCTRL[1]) */
    GCLK_REGS->GCLK_PCHCTRL[1] = GCLK_PCHCTRL_GEN(0x2) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[1] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        /* Wait for synchronization */
    }

    /* Configure DPLL0 */
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

    /* Enable DPLL0 */
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

    /* GCLK0_Initialize(): CPU divider then GEN0 = SRC(7) / 1 */
//    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV(0x01);
//    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV((uint8_t)(BOARD_DPLL0_FREQ / BOARD_CPU_CLOCK));
    /* CPU = GCLK0 / CPU_DIVIDER */
    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV((uint8_t)CPU_DIVIDER);

    while ((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) != MCLK_INTFLAG_CKRDY_Msk)
    {
        /* Wait for Main Clock Ready */
    }
    /* GEN0 = DPLL0 / 1  (CPU clock) */
    GCLK_REGS->GCLK_GENCTRL[0] =
        GCLK_GENCTRL_DIV(GCLK0_DIVIDER) |
        GCLK_GENCTRL_SRC(7) |
        GCLK_GENCTRL_GENEN_Msk;

    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK0) == GCLK_SYNCBUSY_GENCTRL_GCLK0)
    {
        /* wait for Generator 0 synchronization */
    }
    

    /* GCLK1_Initialize(): GEN1 = SRC(7) / 2 */
    /* GEN1 = DPLL0 / GCLK1_DIVIDER (peripheral clock, e.g. UART) */
    GCLK_REGS->GCLK_GENCTRL[1] =
        GCLK_GENCTRL_DIV(GCLK1_DIVIDER) |
        GCLK_GENCTRL_SRC(7) |
        GCLK_GENCTRL_GENEN_Msk;

    while ((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK1) == GCLK_SYNCBUSY_GENCTRL_GCLK1)
    {
        /* wait for Generator 1 synchronization */
    }

    /* Peripheral clock channels exactly like CLOCK_Initialize() */
    /* EIC */
    GCLK_REGS->GCLK_PCHCTRL[4] = GCLK_PCHCTRL_GEN(0x1) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[4] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk) {}

    /* SERCOM2_CORE */
    GCLK_REGS->GCLK_PCHCTRL[23] = GCLK_PCHCTRL_GEN(0x1) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[23] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk) {}

    /* SERCOM3_CORE */
    GCLK_REGS->GCLK_PCHCTRL[24] = GCLK_PCHCTRL_GEN(0x1) | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[24] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk) {}

    /* MCLK masks exactly like CLOCK_Initialize() */
    MCLK_REGS->MCLK_AHBMASK  = 0x00FFFFFFu;
    MCLK_REGS->MCLK_APBAMASK = 0x000007FFu;
    MCLK_REGS->MCLK_APBBMASK = 0x00018656u;
}

void BOARD_Init(void) {

	// LED0: PC18 output, init OFF (active low)
    PORT_DIRSET(LED0_PORT, LED0_MASK);
    LED0_Off();

    // SW0: PB31 input with pull-up (active low)
    PORT_DIRCLR(BUTTON0_PORT, BUTTON0_MASK);

    // Enable input buffer + pull enable (no .bit fields in this header model)
    PORT_PINCFG_SET(BUTTON0_PORT, BUTTON0_PIN, PORT_PINCFG_INEN_PULLEN);

    // Select pull-up: when PULLEN=1, OUT=1 means pull-up
    PORT_OUTSET(BUTTON0_PORT, BUTTON0_MASK);
}

bool led0_is_on(void)
{
    /* Active-low LED on PC18 */
    return ((PORT_REGS->GROUP[LED0_PORT].PORT_OUT & (LED0_MASK)) == 0);
}



static sw_state_t sw0_state = {
    .stable_state = false,
    .last_raw = false,
    .press_start_ms = 0
};

bool sw_pressed(sw_id_t sw, uint32_t debounce)
{
    bool raw;
    sw_state_t *s;
    uint32_t now = millis();

    switch (sw) {
        case SW0:
            raw = SW0_Pressed();
            s = &sw0_state;
            break;
        default:
            return false;
    }

    /* detect raw change */
    if (raw != s->last_raw) {
        s->last_raw = raw;

        if (raw) {
            /* press started */
            s->press_start_ms = now;
        } else {
            /* released */
            s->stable_state = false;
        }
    }

    /* qualify press duration */
    if (raw && !s->stable_state) {
        if ((now - s->press_start_ms) >= debounce) {
            s->stable_state = true;
        }
    }

    return s->stable_state;
}



