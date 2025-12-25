/* qspi_hw.c: SAME54 QSPI peripheral helpers (XC32 SAME54 pack model)
 *
 * Implementation follows the Microchip Harmony QSPI PLIB flow:
 *  - Configure INSTRCTRL/INSTRFRAME
 *  - Access the AHB window at QSPI_ADDR to push/pull payload bytes
 *  - End transfer by setting CTRLA.LASTXFER while keeping ENABLE set
 *  - Wait for INTFLAG.INSTREND and clear it
 */

#include "qspi_hw.h"
#include "../../board.h"

static volatile uint8_t * const QSPI_MEM8 = (volatile uint8_t *)QSPI_AHB_BASE;

static inline uint32_t qspi_optcodelen_field(uint8_t optlen_bits)
{
    switch (optlen_bits)
    {
        case 1:  return QSPI_INSTRFRAME_OPTCODELEN(QSPI_INSTRFRAME_OPTCODELEN_1BIT_Val);
        case 2:  return QSPI_INSTRFRAME_OPTCODELEN(QSPI_INSTRFRAME_OPTCODELEN_2BITS_Val);
        case 4:  return QSPI_INSTRFRAME_OPTCODELEN(QSPI_INSTRFRAME_OPTCODELEN_4BITS_Val);
        case 8:  return QSPI_INSTRFRAME_OPTCODELEN(QSPI_INSTRFRAME_OPTCODELEN_8BITS_Val);
        default: return QSPI_INSTRFRAME_OPTCODELEN(QSPI_INSTRFRAME_OPTCODELEN_8BITS_Val);
    }
}

static inline bool qspi_end_transfer_wait(void)
{
    QSPI_REGS->QSPI_CTRLA = (QSPI_CTRLA_ENABLE_Msk | QSPI_CTRLA_LASTXFER_Msk);

    while ((QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk) == 0U)
    {
        /* spin */
    }

    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;
    return true;
}

void QSPI_HW_Init(void)
{
    /* Enable bus clocks to QSPI */
    MCLK_REGS->MCLK_AHBMASK |= (MCLK_AHBMASK_QSPI_Msk | MCLK_AHBMASK_QSPI_2X_Msk);

    /* Software reset */
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_SWRST_Msk;
    while ((QSPI_REGS->QSPI_CTRLA & QSPI_CTRLA_SWRST_Msk) != 0U)
    {
        /* spin */
    }

    /* Default: Memory mode + CS no-reload (matches Harmony PLIB) */
    QSPI_REGS->QSPI_CTRLB =
        QSPI_CTRLB_MODE_MEMORY |
        QSPI_CTRLB_CSMODE(QSPI_CTRLB_CSMODE_NORELOAD_Val) |
        QSPI_CTRLB_DATALEN(0x6U);

    /* Conservative default baud; caller can tune if needed */
    QSPI_REGS->QSPI_BAUD = QSPI_BAUD_BAUD(1U);

    /* Polling mode */
    QSPI_REGS->QSPI_INTENCLR = 0xFFU;
}

void QSPI_HW_Enable(void)
{
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk;
    while ((QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) == 0U)
    {
        /* spin */
    }
}

void QSPI_HW_Disable(void)
{
    QSPI_REGS->QSPI_CTRLA = 0U;
    while ((QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) != 0U)
    {
        /* spin */
    }
}

void QSPI_HW_ConfigureMemoryRead(uint8_t opcode,
                                 qspi_width_t width,
                                 qspi_addrlen_t addrlen,
                                 uint8_t optcode,
                                 uint8_t optlen_bits,
                                 uint8_t dummy_cycles)
{
    QSPI_REGS->QSPI_INSTRCTRL =
        QSPI_INSTRCTRL_INSTR(opcode) |
        QSPI_INSTRCTRL_OPTCODE(optcode);

    uint32_t frame =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READMEMORY_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen) |
        QSPI_INSTRFRAME_DATAEN_Msk |
        QSPI_INSTRFRAME_DUMMYLEN(dummy_cycles);

    if (optlen_bits != 0U)
    {
        frame |= QSPI_INSTRFRAME_OPTCODEEN_Msk;
        frame |= qspi_optcodelen_field(optlen_bits);
    }

    QSPI_REGS->QSPI_INSTRFRAME = frame;
}

bool QSPI_HW_Command(uint8_t opcode, qspi_width_t width)
{
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk;

    return qspi_end_transfer_wait();
}

bool QSPI_HW_Read(uint8_t opcode, qspi_width_t width, void *rx, size_t rx_len)
{
    if ((rx == NULL) || (rx_len == 0U))
    {
        return false;
    }

    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk;

    __DSB();
    __ISB();

    uint8_t *dst = (uint8_t *)rx;
    for (size_t i = 0; i < rx_len; i++)
    {
        dst[i] = (uint8_t)QSPI_MEM8[i];
    }

    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}

bool QSPI_HW_Write(uint8_t opcode, qspi_width_t width, const void *tx, size_t tx_len)
{
    if ((tx == NULL) || (tx_len == 0U))
    {
        return false;
    }

    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_WRITE_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk;

    __DSB();
    __ISB();

    const uint8_t *src = (const uint8_t *)tx;
    for (size_t i = 0; i < tx_len; i++)
    {
        QSPI_MEM8[i] = src[i];
    }

    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}
