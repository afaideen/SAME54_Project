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

static inline bool qspi_wait_instrend_clear(void)
{
    uint32_t guard = 2000000UL;
    while (((QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk) == 0U) && (guard-- != 0U)) { }
    if (guard == 0U)
        return false;

    /* Harmony clears AFTER observe */
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;
    return true;
}


/* AHB aperture base (device-pack symbol) */
static volatile uint8_t * const QSPI_MEM8 = (volatile uint8_t *)QSPI_ADDR;

/* Harmony-style transfer prologue:
 *  - define INSTRADDR even for register commands
 *  - clear stale INSTREND so next transfer isn't ignored
 */
static inline void qspi_begin_transfer_common(void)
{
    if ((QSPI_REGS->QSPI_CTRLA & QSPI_CTRLA_ENABLE_Msk) == 0U)
    {
        QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_ENABLE_Msk;
    }

    /* Harmony always defines INSTRADDR (0 for pure register commands). */
    QSPI_REGS->QSPI_INSTRADDR = 0U;

    /* Clear any stale completion so the next transfer can complete cleanly. */
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;
}

static inline void QSPI_HW_SyncInstr(void)
{
    (void)QSPI_REGS->QSPI_INSTRFRAME;
}

/* End-of-transfer + wait complete (Harmony style) */
static inline bool qspi_end_transfer_wait(void)
{
    /* End transfer */
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk | QSPI_CTRLA_LASTXFER_Msk;

    uint32_t guard = 2000000UL;
    while (((QSPI_REGS->QSPI_INTFLAG & QSPI_INTFLAG_INSTREND_Msk) == 0U) && (guard-- != 0U)) { }
    if (guard == 0U) return false;

    /* Clear AFTER observe */
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;
    return true;
}



/* Small memcpy helpers (Harmony-like word then byte tail copy) */
static inline void qspi_memcpy_32bit(volatile uint32_t *dst, const uint32_t *src, size_t words)
{
    for (size_t i = 0; i < words; i++)
    {
        dst[i] = src[i];
    }
}

static inline void qspi_memcpy_8(volatile uint8_t *dst, const uint8_t *src, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        dst[i] = src[i];
    }
}

void QSPI_HW_PinInit(void)
{
    /* Enable PORT bus clock (PORT is on APBB) */
    MCLK_REGS->MCLK_APBBMASK |= MCLK_APBBMASK_PORT_Msk;

    /* ===== PORTA: PA08..PA11 -> Function H (QSPI DATA0..DATA3) =====
       Harmony:
         PINCFG[8..11] = 0x1
         PMUX[4] = 0x77 (PA08/PA09)
         PMUX[5] = 0x77 (PA10/PA11)
    */
    PORT_REGS->GROUP[0].PORT_PINCFG[8]  = 0x1U;
    PORT_REGS->GROUP[0].PORT_PINCFG[9]  = 0x1U;
    PORT_REGS->GROUP[0].PORT_PINCFG[10] = 0x1U;
    PORT_REGS->GROUP[0].PORT_PINCFG[11] = 0x1U;

    PORT_REGS->GROUP[0].PORT_PMUX[4] = 0x77U;
    PORT_REGS->GROUP[0].PORT_PMUX[5] = 0x77U;

    /* ===== PORTB: PB10..PB11 -> Function H (QSPI SCK / CS) =====
       Harmony:
         PINCFG[10..11] = 0x1
         PMUX[5] = 0x77 (PB10/PB11)
    */
    PORT_REGS->GROUP[1].PORT_PINCFG[10] = 0x1U;
    PORT_REGS->GROUP[1].PORT_PINCFG[11] = 0x1U;

    PORT_REGS->GROUP[1].PORT_PMUX[5] = 0x77U;
}

void QSPI_HW_Initialize(void)
{
    /* Reset and Disable the qspi Module */
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_SWRST_Msk;

    // Set Mode Register values
    /* MODE = MEMORY */
    /* LOOPEN = 0 */
    /* WDRBT = 0 */
    /* SMEMREG = 0 */
    /* CSMODE = NORELOAD */
    /* DATALEN = 0x6 */
    /* DLYCBT = 0 */
    /* DLYCS = 0 */
    QSPI_REGS->QSPI_CTRLB = QSPI_CTRLB_MODE_MEMORY | QSPI_CTRLB_CSMODE_NORELOAD | QSPI_CTRLB_DATALEN(0x6U);

    // Set serial clock register
    QSPI_REGS->QSPI_BAUD = (QSPI_BAUD_BAUD(1U))  ;

    // Enable the qspi Module
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_ENABLE_Msk;

    while((QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) != QSPI_STATUS_ENABLE_Msk)
    {
        /* Wait for QSPI enable flag to set */
    }
}

void QSPI_HW_Init(void)
{
    /* 0) Enable bus clocks needed for QSPI register + AHB aperture access */
    MCLK_REGS->MCLK_AHBMASK  |= (MCLK_AHBMASK_QSPI_Msk | MCLK_AHBMASK_QSPI_2X_Msk);
    MCLK_REGS->MCLK_APBCMASK |= MCLK_APBCMASK_QSPI_Msk;

    /* 1) Pin mux (matches Harmony PORT_Initialize) */
    // PA08..PA11 = QIO0..3, PB10=QSCK, PB11=QCS (PMUX H)
    // ... your existing pinmux code ...

    /* 2) Software reset */
    QSPI_REGS->QSPI_CTRLA = QSPI_CTRLA_SWRST_Msk;
    while ((QSPI_REGS->QSPI_CTRLA & QSPI_CTRLA_SWRST_Msk) != 0U) { }

    /* 3) Match Harmony: MODE = MEMORY, CSMODE = NORELOAD, DATALEN = 0x6 */
    QSPI_REGS->QSPI_CTRLB =
        QSPI_CTRLB_MODE_MEMORY |
        QSPI_CTRLB_CSMODE(QSPI_CTRLB_CSMODE_NORELOAD_Val) |
        QSPI_CTRLB_DATALEN(0x6U);

    /* 4) BAUD */
    QSPI_REGS->QSPI_BAUD = QSPI_BAUD_BAUD(1U);

    /* 5) Polling mode */
    QSPI_REGS->QSPI_INTENCLR = 0xFFU;
}


void QSPI_HW_Enable(void)
{
    QSPI_REGS->QSPI_CTRLA |= QSPI_CTRLA_ENABLE_Msk;
    while ((QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) == 0U) { }
}

void QSPI_HW_Disable(void)
{
    QSPI_REGS->QSPI_CTRLA &= ~QSPI_CTRLA_ENABLE_Msk;
}

void QSPI_HW_SetBaud(uint8_t baud_div)
{
    QSPI_REGS->QSPI_BAUD = QSPI_BAUD_BAUD((uint32_t)baud_div);
}

bool QSPI_HW_Command(uint8_t opcode, qspi_width_t width)
{
    qspi_begin_transfer_common();

    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val);

    /* Harmony sync point is readback only */
    (void)QSPI_REGS->QSPI_INSTRFRAME;

    return qspi_wait_instrend_clear();
}

bool QSPI_HW_Read(uint8_t opcode, qspi_width_t width, void *rx, size_t rx_len)
{
    if ((rx == NULL) || (rx_len == 0U))
    {
        return false;
    }

    qspi_begin_transfer_common();

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

    qspi_begin_transfer_common();

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

bool QSPI_HW_CommandAddr(uint8_t opcode,
                         qspi_width_t width,
                         qspi_addrlen_t addrlen,
                         uint32_t address)
{
    qspi_begin_transfer_common(); // important: enable + clear stale INSTREND

    QSPI_REGS->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(address);
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val);

    (void)QSPI_REGS->QSPI_INSTRFRAME;

    return qspi_wait_instrend_clear();
}

bool QSPI_HW_ReadEx(uint8_t opcode,
                    qspi_width_t width,
                    uint8_t dummy_cycles,
                    void *rx,
                    size_t rx_len)
{
    if ((rx == NULL) || (rx_len == 0U))
    {
        return false;
    }

    qspi_begin_transfer_common();

    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode);

    QSPI_REGS->QSPI_INSTRFRAME =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READ_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_DATAEN_Msk |
        QSPI_INSTRFRAME_DUMMYLEN((uint32_t)dummy_cycles);

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

bool QSPI_HW_MemoryRead_Simple(
    uint8_t opcode,
    qspi_width_t width,
    uint8_t dummy_cycles,
    uint32_t address,
    void *rx,
    size_t rx_len
)
{
    return QSPI_HW_MemoryRead(
        opcode,                     /* opcode decided by flash driver */
        width,                      /* bus width */
        QSPI_ADDRLEN_24BITS,        /* standard 24-bit address */
        false,                      /* no option code */
        0,                          /* optcode */
        0,                          /* optlen */
        dummy_cycles,               /* dummy cycles */
        rx,
        rx_len,
        address
    );
}


bool QSPI_HW_MemoryRead(uint8_t opcode,
                        qspi_width_t width,
                        qspi_addrlen_t addrlen,
                        bool opt_en,
                        uint8_t optcode,
                        uint8_t optlen_bits,
                        uint8_t dummy_cycles,
                        void *rx,
                        size_t rx_len,
                        uint32_t address)
{
    if ((rx == NULL) || (rx_len == 0U))
    {
        return false;
    }

    /* Clear stale completion before starting a new instruction */
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;

    /* Program address/opcode */
    QSPI_REGS->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(address);
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode) |
                                (opt_en ? QSPI_INSTRCTRL_OPTCODE(optcode) : 0U);


    uint32_t frame =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_READMEMORY_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen) |
        QSPI_INSTRFRAME_DATAEN_Msk;

    if (opt_en)
    {
        frame |= QSPI_INSTRFRAME_OPTCODEEN_Msk;
        frame |= QSPI_INSTRFRAME_OPTCODELEN((uint32_t)optlen_bits);
    }

    if (dummy_cycles != 0U)
    {
        frame |= QSPI_INSTRFRAME_DUMMYLEN((uint32_t)dummy_cycles);
    }

    QSPI_REGS->QSPI_INSTRFRAME = frame;
    QSPI_HW_SyncInstr();

    /* Read via AHB window */
    uint8_t *dst8 = (uint8_t *)rx;
    const volatile uint8_t *src8 = (const volatile uint8_t *)(QSPI_ADDR | address);

    for (size_t i = 0; i < rx_len; i++)
    {
        dst8[i] = src8[i];
    }

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
    if ((tx == NULL) || (tx_len == 0U))
    {
        return false;
    }

    /* Clear stale completion before starting a new instruction */
    QSPI_REGS->QSPI_INTFLAG = QSPI_INTFLAG_INSTREND_Msk;

    QSPI_REGS->QSPI_INSTRADDR = QSPI_INSTRADDR_ADDR(address);
    QSPI_REGS->QSPI_INSTRCTRL = QSPI_INSTRCTRL_INSTR(opcode) |
                                (opt_en ? QSPI_INSTRCTRL_OPTCODE(optcode) : 0U);


    uint32_t frame =
        QSPI_INSTRFRAME_WIDTH((uint32_t)width) |
        QSPI_INSTRFRAME_TFRTYPE(QSPI_INSTRFRAME_TFRTYPE_WRITEMEMORY_Val) |
        QSPI_INSTRFRAME_INSTREN_Msk |
        QSPI_INSTRFRAME_ADDREN_Msk |
        QSPI_INSTRFRAME_ADDRLEN((uint32_t)addrlen) |
        QSPI_INSTRFRAME_DATAEN_Msk;

    if (opt_en)
    {
        frame |= QSPI_INSTRFRAME_OPTCODEEN_Msk;
        frame |= QSPI_INSTRFRAME_OPTCODELEN((uint32_t)optlen_bits);
    }

    if (dummy_cycles != 0U)
    {
        frame |= QSPI_INSTRFRAME_DUMMYLEN((uint32_t)dummy_cycles);
    }

    QSPI_REGS->QSPI_INSTRFRAME = frame;
    QSPI_HW_SyncInstr();

    /* Write payload into AHB aperture (word then byte tail like Harmony) */
    volatile uint32_t *dst32 = (volatile uint32_t *)(QSPI_ADDR | address);
    const uint32_t *src32 = (const uint32_t *)tx;
    size_t n32 = tx_len / 4U;

    if (n32 != 0U)
    {
        qspi_memcpy_32bit(dst32, src32, n32);
    }

    volatile uint8_t *dst8 = (volatile uint8_t *)(QSPI_ADDR | address);
    const uint8_t *src8 = (const uint8_t *)tx;

    size_t done = n32 * 4U;
    if (done < tx_len)
    {
        qspi_memcpy_8(dst8 + done, src8 + done, tx_len - done);
    }

    __DSB();
    __ISB();

    return qspi_end_transfer_wait();
}
