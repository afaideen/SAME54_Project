
#include <stdio.h>
#include "qspi_flash.h"
#include "qspi_hw.h"



#if APP_USE_SST26_FLASH
#include "sst26/sst26.h"
#elif APP_USE_N25Q_FLASH
#include "n25q/n25q256a.h"
#endif
/*
 * QSPI flash init sequence (SAME54 Xplained Pro + N25Q256A):
 *  1) Init QSPI peripheral (AHB clocks + reset + basic CTRLB/BAUD)
 *  2) Reset flash (66h/99h)
 *  3) Read JEDEC ID (9Fh)
 *  4) Enable Quad I/O via Enhanced Volatile Config Register (61h)
 *  5) Program Volatile Config Register dummy cycles (81h)
 *  6) Enter 4-byte addressing (B7h)
 *  7) Configure SAME54 QSPI memory-read instruction frame for XIP-style reads
 */
bool QSPI_Flash_Init(void)
{
#if APP_USE_SST26_FLASH
    uint32_t jedec = 0;

    /* Harmony: QSPI_Initialize() */
    QSPI_HW_Init();
    QSPI_HW_Enable();

    /* Harmony APP_STATE_RESET_FLASH */
    if (!SST26_Reset())
    {
        printf("[QSPI] SST26_Reset failed\r\n");
        return false;
    }

    /* Harmony APP_STATE_ENABLE_QUAD_IO */
    if (!SST26_EnableQuadIO())
    {
        printf("[QSPI] SST26_EnableQuadIO failed\r\n");
        return false;
    }

    /* Harmony APP_STATE_UNLOCK_FLASH (WREN + Global Unprotect) */
    if (!SST26_UnlockGlobal())
    {
        printf("[QSPI] SST26_UnlockGlobal failed\r\n");
        return false;
    }

    /* Harmony APP_STATE_READ_JEDEC_ID uses 0xAF (QUAD), dummy=2, len=3 */
    if (!SST26_ReadJEDEC(&jedec))
    {
        printf("[QSPI] SST26_ReadJEDEC failed\r\n");
        return false;
    }

    printf("QSPI: JEDEC ID = 0x%06lX\r\n", (unsigned long)(jedec & 0x00FFFFFFUL));

    if (jedec != SST26VF064B_JEDEC_ID)
    {
        printf("QSPI: JEDEC mismatch. expected=0x%06lX\r\n",
               (unsigned long)(SST26VF064B_JEDEC_ID & 0x00FFFFFFUL));
        return false;
    }

  

    return true;

#elif APP_USE_N25Q_FLASH
    /* (keep your old N25Q flow here if you want) */
    return false;
#endif
}



static const char *qspi_width_str(uint32_t width)
{
    switch (width)
    {
        case 0: return "1-1-1";
        case 1: return "1-1-2";
        case 2: return "1-1-4";
        case 3: return "1-2-2";
        case 4: return "1-4-4";
        case 5: return "2-2-2";
        case 6: return "4-4-4";
        default: return "UNKNOWN";
    }
}

static const char *qspi_tfrtype_str(uint32_t tfr)
{
    /* Values are encoded in the device header as _Val enums; we print raw if unknown */
    switch (tfr)
    {
        case QSPI_INSTRFRAME_TFRTYPE_READ_Val:        return "READ (REG)";
        case QSPI_INSTRFRAME_TFRTYPE_WRITE_Val:       return "WRITE (REG)";
        case QSPI_INSTRFRAME_TFRTYPE_READMEMORY_Val:  return "READMEMORY (XIP)";
        case QSPI_INSTRFRAME_TFRTYPE_WRITEMEMORY_Val: return "WRITEMEMORY (XIP)";
        default: return "TFRTYPE ?";
    }
}

void QSPI_Flash_Diag_Print(void)
{
    printf("\r\n");
    printf("=============== QSPI DIAGNOSTIC ===============\r\n");

    /* ---- QSPI peripheral state (use STATUS, not CTRLA) ---- */
    printf("QSPI Peripheral : %s\r\n",
        (QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) ? "ENABLED" : "DISABLED");

    printf("QSPI Mode       : %s\r\n",
        (QSPI_REGS->QSPI_CTRLB & QSPI_CTRLB_MODE_MEMORY) ? "MEMORY (XIP)" : "SPI/REG");

    /* ---- QSPI BAUD decode (field is at bit[15:8]) ---- */
    uint32_t baud_field = (QSPI_REGS->QSPI_BAUD & QSPI_BAUD_BAUD_Msk) >> QSPI_BAUD_BAUD_Pos;

    /*
     * QSPI SCK formula is device-defined; on SAM D5x/E5x it is commonly:
     *   f_sck ~= f_qspi / (2 * (BAUD + 1))
     *
     * We don't have a direct "QSPI clock source" reader here, so we assume QSPI is
     * sourced from the same high-speed clock domain as the CPU (your log shows 120 MHz).
     */
#ifdef CPU_CLOCK_HZ
    uint32_t f_qspi = (uint32_t)CPU_CLOCK_HZ;
#else
    uint32_t f_qspi = 120000000UL;
#endif
    uint32_t f_sck = f_qspi / (2UL * (baud_field + 1UL));

    printf("QSPI BAUD       : BAUD=%lu  (~%lu.%03lu MHz)\r\n",
        baud_field,
        f_sck / 1000000UL,
        (f_sck % 1000000UL) / 1000UL);

    /* ---- Current instruction frame (what the MCU is set up to do) ---- */
    uint32_t instr = QSPI_REGS->QSPI_INSTRCTRL;
    uint32_t frame = QSPI_REGS->QSPI_INSTRFRAME;

    uint8_t opcode  = (uint8_t)((instr & QSPI_INSTRCTRL_INSTR_Msk) >> QSPI_INSTRCTRL_INSTR_Pos);
    uint8_t optcode = (uint8_t)((instr & QSPI_INSTRCTRL_OPTCODE_Msk) >> QSPI_INSTRCTRL_OPTCODE_Pos);

    uint32_t width  = (frame & QSPI_INSTRFRAME_WIDTH_Msk) >> QSPI_INSTRFRAME_WIDTH_Pos;
    uint32_t tfr    = (frame & QSPI_INSTRFRAME_TFRTYPE_Msk) >> QSPI_INSTRFRAME_TFRTYPE_Pos;
    uint32_t addrlen= (frame & QSPI_INSTRFRAME_ADDRLEN_Msk) >> QSPI_INSTRFRAME_ADDRLEN_Pos;
    uint32_t dummy  = (frame & QSPI_INSTRFRAME_DUMMYLEN_Msk) >> QSPI_INSTRFRAME_DUMMYLEN_Pos;
    uint32_t optlen = (frame & QSPI_INSTRFRAME_OPTCODELEN_Msk) >> QSPI_INSTRFRAME_OPTCODELEN_Pos;

    printf("Mem Opcode      : 0x%02X\r\n", opcode);
    printf("Mem Width       : %s\r\n", qspi_width_str(width));
    printf("Mem TfrType     : %s\r\n", qspi_tfrtype_str(tfr));
    printf("Mem AddrLen     : %s\r\n", (addrlen == 3U) ? "32-bit" : "24-bit/other");
    printf("Mem OptCode     : 0x%02X (len=%lu)\r\n", optcode, optlen);
    printf("Mem Dummy       : %lu\r\n", dummy);

    /* ---- JEDEC ID ---- */
    /* Always perform a direct 0x9F read in register mode to determine manufacturer. */
    uint8_t id[3] = {0};
    uint8_t mfr = 0xFFU;
    if (QSPI_HW_Read(0x9FU, QSPI_WIDTH_SINGLE_BIT_SPI, &id[0], sizeof(id)))
    {
        mfr  = id[0];
        uint8_t type = id[1];
        uint8_t cap  = id[2];
        printf("JEDEC ID        : 0x%02X %02X %02X\r\n", mfr, type, cap);

        if (mfr == 0x20U)
            printf("Flash Detected  : Micron (N25Q family)\r\n");
        else if (mfr == 0xBFU)
            printf("Flash Detected  : SST/Microchip (SST26)\r\n");
        else if (mfr == 0xFFU || mfr == 0x00U)
            printf("Flash Detected  : INVALID (check wiring/CS/clock)\r\n");
        else
            printf("Flash Detected  : Unknown MFR\r\n");
    }
    else
    {
        printf("JEDEC ID        : READ FAILED\r\n");
    }

    /* ---- Status register ---- */
    /* ---- Status register ---- */
#if APP_USE_N25Q_FLASH
    if (mfr == 0x20U)
    {
        uint8_t sr = N25Q_ReadStatus();
        printf("SR              : 0x%02X\r\n", sr);
        printf("Flash Ready     : %s\r\n",
               (sr & N25Q_SR_WIP_Msk) ? "NO (WIP=1)" : "YES");
        printf("Write Enable    : %s\r\n",
               (sr & N25Q_SR_WEL_Msk) ? "YES (WEL=1)" : "NO (WEL=0)");

        /* ... NVCR/VCR/EVCR ... */
        return;
    }
#endif

#if APP_USE_SST26_FLASH
    if (mfr == 0xBFU)
    {
        uint8_t sr = 0U;
        if (SST26_ReadStatus(&sr))
        {
            printf("SR              : 0x%02X\r\n", sr);
            printf("Flash Ready     : %s\r\n",
                   (sr & SST26_SR_WIP_Msk) ? "NO (WIP=1)" : "YES");
        }
        else
        {
            printf("SR              : READ FAILED\r\n");
            printf("Flash Ready     : UNKNOWN\r\n");
        }
        return;
    }
#endif

    /* If we get here, either flash is unknown or that driver is not compiled in */
    printf("SR              : UNKNOWN\r\n");
    printf("Flash Ready     : UNKNOWN\r\n");


    printf("===============================================\r\n");
}

