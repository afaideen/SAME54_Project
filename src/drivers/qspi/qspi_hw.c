/* qspi_hw.c: Implementation of QSPI low-level interface for SAME54 */
#include "sam.h"
#include "qspi_hw.h"
#include "board.h"

/* ---------- low-level helpers ---------- */

static void qspi_wait_sync(uint32_t mask)
{
    while (QSPI_REGS->QSPI_SYNCBUSY & mask);
}

/* ---------- hardware init ---------- */

void QSPI_HW_Init(void)
{
    /* Enable QSPI clocks */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_QSPI_Msk;
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_QSPI_Msk;

    /* Configure pins: PA08–PA11, PB10–PB11 */
    const uint8_t pa_pins[] = {8, 9, 10, 11};
    const uint8_t pb_pins[] = {10, 11};

    for (unsigned i = 0; i < 4; i++) {
        uint8_t p = pa_pins[i];
        PORT_REGS->GROUP[0].PORT_PINCFG[p] =
            PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_DRVSTR_Msk;

        if ((p & 1) == 0)
            PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] =
                (PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] &
                 ~PORT_PMUX_PMUXE_Msk) | PORT_PMUX_PMUXE_H;
        else
            PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] =
                (PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] &
                 ~PORT_PMUX_PMUXO_Msk) | PORT_PMUX_PMUXO_H;
    }

    for (unsigned i = 0; i < 2; i++) {
        uint8_t p = pb_pins[i];
        PORT_REGS->GROUP[1].PORT_PINCFG[p] =
            PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_DRVSTR_Msk;

        if ((p & 1) == 0)
            PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] =
                (PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] &
                 ~PORT_PMUX_PMUXE_Msk) | PORT_PMUX_PMUXE_H;
        else
            PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] =
                (PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] &
                 ~PORT_PMUX_PMUXO_Msk) | PORT_PMUX_PMUXO_H;
    }

    /* Software reset */
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_SWRST_Msk;
    qspi_wait_sync(QSPI_SYNCBUSY_SWRST_Msk);
}

void QSPI_HW_Enable(void)
{
    QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_ENABLE_Msk;
    qspi_wait_sync(QSPI_SYNCBUSY_ENABLE_Msk);
}

/* ---------- XIP ---------- */

void QSPI_HW_EnableXIP(void)
{
    QSPI_REGS->QSPI_CTRLB =
        QSPI_CTRLB_MODE_MEMORY |
        QSPI_CTRLB_CSMODE_NORELOAD;

    qspi_wait_sync(QSPI_SYNCBUSY_CTRLB_Msk);
}

void QSPI_HW_DisableXIP(void)
{
    QSPI_REGS->QSPI_CTRLB &= ~QSPI_CTRLB_MODE_Msk;
    qspi_wait_sync(QSPI_SYNCBUSY_CTRLB_Msk);
}

/* ---------- command helpers ---------- */

bool QSPI_Command(uint8_t opcode)
{
    QSPI_REGS->QSPI_INSTRCTRL = opcode;
    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_TFRTYPE_READ;

    QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_LASTXFER_Msk;

    while (!(QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk));
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;

    return true;
}

bool QSPI_CommandRead(uint8_t opcode, void *buf, uint32_t len)
{
    QSPI_REGS->QSPI_INSTRCTRL = opcode;
    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk |
        QSPI_INSTRFRAME_TFRTYPE_READ;

    volatile uint8_t *mem = (volatile uint8_t *)QSPI_AHB_BASE;
    for (uint32_t i = 0; i < len; i++)
        ((uint8_t *)buf)[i] = mem[i];

    QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_LASTXFER_Msk;

    while (!(QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk));
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;

    return true;
}

bool QSPI_CommandWrite(uint8_t opcode, const void *buf, uint32_t len)
{
    QSPI_REGS->QSPI_INSTRCTRL = opcode;
    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk |
        QSPI_INSTRFRAME_TFRTYPE_WRITE;

    volatile uint8_t *mem = (volatile uint8_t *)QSPI_AHB_BASE;
    for (uint32_t i = 0; i < len; i++)
        mem[i] = ((const uint8_t *)buf)[i];

    QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_LASTXFER_Msk;

    while (!(QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk));
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;

    return true;
}