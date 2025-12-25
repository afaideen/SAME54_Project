/* qspi_hw.c: SAME54 QSPI peripheral helpers (XC32 SAME54 pack model)
 *
 * Implementation follows the Microchip Harmony QSPI PLIB flow:
 *  - Configure INSTRCTRL/INSTRFRAME
 *  - Access the AHB window at QSPI_ADDR to push/pull payload bytes
 *  - End transfer by setting CTRLA.LASTXFER while keeping ENABLE set
 *  - Wait for INTFLAG.INSTREND and clear it
 */

#include "qspi_hw.h"
//#include "../../common/board.h"

/* AHB aperture base (device-pack symbol) */
static volatile uint8_t * const QSPI_MEM8 = (volatile uint8_t *)QSPI_ADDR;

static inline void QSPI_HW_SyncInstr(void)
{
    /* Harmony PLIB pattern: read-back to ensure APB writes are visible before AHB access */
    (void)QSPI_REGS->QSPI_INSTRFRAME;
    __DSB();
    __ISB();
}

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
    /* Clear stale completion */
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;

    /* Don’t overwrite CTRLA; just request end-of-transfer */
    QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_LASTXFER_Msk;

    uint32_t guard = 2000000UL;
    while (((QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk) == 0U) && (guard-- != 0U)) { }
    if (guard == 0U) return false;

    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;
    return true;
}


void QSPI_HW_Init(void)
{
    /* Enable bus clocks to QSPI */
    MCLK_REGS->MCLK_AHBMASK |= (MCLK_AHBMASK_QSPI_Msk | MCLK_AHBMASK_QSPI_2X_Msk);

    /* Some header packs also gate QSPI on APBC. Enable if present. */
#ifdef MCLK_APBCMASK_QSPI_Msk
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_QSPI_Msk;
#endif

    /* Configure QSPI pins (Harmony-equivalent):
     *  - PA08..PA11 : QSPI DATA[0..3]
     *  - PB10       : QSPI SCK
     *  - PB11       : QSPI CS
     * Peripheral function = H, enable PMUX, enable high drive strength.
     */
    const uint8_t pa_pins[] = { 8U, 9U, 10U, 11U };
    for (size_t i = 0; i < (sizeof(pa_pins) / sizeof(pa_pins[0])); i++)
    {
        const uint8_t p = pa_pins[i];

        PORT_REGS->GROUP[0].PORT_PINCFG[p] =
            (uint8_t)(PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_DRVSTR_Msk);

        if ((p & 1U) == 0U)
        {
            PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] =
                (uint8_t)((PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] & (uint8_t)~PORT_PMUX_PMUXE_Msk) |
                          (uint8_t)PORT_PMUX_PMUXE_H);
        }
        else
        {
            PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] =
                (uint8_t)((PORT_REGS->GROUP[0].PORT_PMUX[p >> 1] & (uint8_t)~PORT_PMUX_PMUXO_Msk) |
                          (uint8_t)PORT_PMUX_PMUXO_H);
        }
    }

    const uint8_t pb_pins[] = { 10U, 11U };
    for (size_t i = 0; i < (sizeof(pb_pins) / sizeof(pb_pins[0])); i++)
    {
        const uint8_t p = pb_pins[i];

        PORT_REGS->GROUP[1].PORT_PINCFG[p] =
            (uint8_t)(PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_DRVSTR_Msk);

        if ((p & 1U) == 0U)
        {
            PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] =
                (uint8_t)((PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] & (uint8_t)~PORT_PMUX_PMUXE_Msk) |
                          (uint8_t)PORT_PMUX_PMUXE_H);
        }
        else
        {
            PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] =
                (uint8_t)((PORT_REGS->GROUP[1].PORT_PMUX[p >> 1] & (uint8_t)~PORT_PMUX_PMUXO_Msk) |
                          (uint8_t)PORT_PMUX_PMUXO_H);
        }
    }

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
    QSPI_HW_SyncInstr();
}

bool QSPI_HW_Command(uint8_t opcode, qspi_width_t width)
{
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk;
    QSPI_HW_SyncInstr();

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

    QSPI_HW_SyncInstr();

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

    QSPI_HW_SyncInstr();

    const uint8_t *src = (const uint8_t *)tx;
    for (size_t i = 0; i < tx_len; i++)
    {
        QSPI_MEM8[i] = src[i];
    }

    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}

static inline void qspi_memcpy_8(uint8_t *dst, const uint8_t *src, size_t n)
{
    while (n-- != 0U) { *dst++ = *src++; }
}

static inline void qspi_memcpy_32(uint32_t *dst, const uint32_t *src, size_t n_words)
{
    while (n_words-- != 0U) { *dst++ = *src++; }
}

bool QSPI_HW_CommandAddr(uint8_t opcode,
                         qspi_width_t width,
                         qspi_addrlen_t addrlen,
                         uint32_t address)
{
    QSPI_REGS->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(address);
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen);

    QSPI_HW_SyncInstr();
    return qspi_end_transfer_wait();
}

bool QSPI_HW_ReadEx(uint8_t opcode,
                    qspi_width_t width,
                    uint8_t dummy_cycles,
                    void *rx,
                    size_t rx_len)
{
    if ((rx == NULL) || (rx_len == 0U)) { return false; }

    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_DUMMYLEN(dummy_cycles) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk;

    QSPI_HW_SyncInstr();

    uint8_t *dst = (uint8_t *)rx;
    for (size_t i = 0; i < rx_len; i++) { dst[i] = (uint8_t)QSPI_MEM8[i]; }

    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}

bool QSPI_HW_MemoryRead(uint8_t opcode,
                        qspi_width_t width,
                        qspi_addrlen_t addrlen,
                        bool opt_en,
                        uint8_t optcode,
                        uint8_t optlen_bits,
                        bool continuous_read_en,
                        uint8_t dummy_cycles,
                        void *rx,
                        size_t rx_len,
                        uint32_t address)
{
    if ((rx == NULL) || (rx_len == 0U)) { return false; }

    QSPI_REGS->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(address);
    QSPI_REGS->QSPI_INSTRCTRL =
        QSPI_INSTRCTRL_INSTR(opcode) |
        QSPI_INSTRCTRL_OPTCODE(optcode);

    uint32_t frame =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen) |
        QSPI_INSTRFRAME_DUMMYLEN(dummy_cycles) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READMEMORY_Val);

    if (opt_en)
    {
        frame |= QSPI_INSTRFRAME_OPTCODEEN_Msk;
        frame |= qspi_optcodelen_field(optlen_bits);
    }

    if (continuous_read_en)
    {
        frame |= QSPI_INSTRFRAME_CRMODE_Msk;
    }

    QSPI_REGS->QSPI_INSTRFRAME = frame;
    QSPI_HW_SyncInstr();

    /* Use the AHB-mapped window + address (same pattern as Harmony PLIB) */
    const uintptr_t src_addr = (uintptr_t)QSPI_ADDR | (uintptr_t)address;
    const uint8_t *src8 = (const uint8_t *)src_addr;

    /* Safe copy: do 32-bit only if both aligned */
    uintptr_t dst_addr = (uintptr_t)rx;
    size_t n32 = 0;

    if (((src_addr | dst_addr) & 0x3U) == 0U)
    {
        n32 = rx_len / 4U;
        qspi_memcpy_32((uint32_t *)rx, (const uint32_t *)src_addr, n32);
    }

    size_t done = n32 * 4U;
    if (done < rx_len)
    {
        qspi_memcpy_8((uint8_t *)rx + done, src8 + done, rx_len - done);
    }

    (void)QSPI_REGS->QSPI_INTFLAG; /* Harmony does a dummy read here */
    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}

bool QSPI_HW_MemoryWrite(uint8_t opcode,
                         qspi_width_t width,
                         qspi_addrlen_t addrlen,
                         bool opt_en,
                         uint8_t optcode,
                         uint8_t optlen_bits,
                         uint8_t dummy_cycles,
                         const void *tx,
                         size_t tx_len,
                         uint32_t address)
{
    if ((tx == NULL) || (tx_len == 0U)) { return false; }

    QSPI_REGS->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(address);
    QSPI_REGS->QSPI_INSTRCTRL =
        QSPI_INSTRCTRL_INSTR(opcode) |
        QSPI_INSTRCTRL_OPTCODE(optcode);

    uint32_t frame =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen) |
        QSPI_INSTRFRAME_DUMMYLEN(dummy_cycles) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_WRITEMEMORY_Val);

    if (opt_en)
    {
        frame |= QSPI_INSTRFRAME_OPTCODEEN_Msk;
        frame |= qspi_optcodelen_field(optlen_bits);
    }

    QSPI_REGS->QSPI_INSTRFRAME = frame;
    QSPI_HW_SyncInstr();

    const uintptr_t dst_addr = (uintptr_t)QSPI_ADDR | (uintptr_t)address;
    uint8_t *dst8 = (uint8_t *)dst_addr;
    const uint8_t *src8 = (const uint8_t *)tx;

    /* Safe copy: 32-bit only if both aligned */
    uintptr_t src_addr = (uintptr_t)tx;
    size_t n32 = 0;

    if (((dst_addr | src_addr) & 0x3U) == 0U)
    {
        n32 = tx_len / 4U;
        qspi_memcpy_32((uint32_t *)dst_addr, (const uint32_t *)tx, n32);
    }

    size_t done = n32 * 4U;
    if (done < tx_len)
    {
        qspi_memcpy_8(dst8 + done, src8 + done, tx_len - done);
    }

    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}



