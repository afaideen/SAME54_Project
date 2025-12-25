
#include <stdio.h>
#include "qspi_flash.h"
#include "qspi_hw.h"
#include "n25q/n25q256a.h"
#include "sst26/sst26.h"


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
    uint32_t jedec;
    uint8_t mfr;

    QSPI_HW_Init();
    QSPI_HW_Enable();

    if (!QSPI_HW_Read(0x9F, QSPI_WIDTH_SINGLE_BIT_SPI, &jedec, 3))
        return false;

    mfr = (uint8_t)(jedec & 0xFF);

    if (mfr == 0xBF)
    {
        /* -------- SST26 -------- */
        if (!SST26_Reset()) return false;
        if (!SST26_EnableQuadIO()) return false;
        if (!SST26_UnlockGlobal()) return false;

        if (!SST26_ReadJEDEC(&jedec)) return false;

        QSPI_HW_ConfigureMemoryRead(
            SST26_CMD_HIGH_SPEED_READ,
            QSPI_WIDTH_QUAD_CMD,
            QSPI_ADDRLEN_24BITS,
            0, 0, 6);

        return true;
    }
    else if (mfr == 0x20)
    {
        /* -------- N25Q -------- */
        if (!N25Q_Reset()) return false;
        if (!N25Q_EnableQuadIO(2000000)) return false;
        if (!N25Q_SetDummyCycles(10, 2000000)) return false;
        if (!N25Q_Enter4ByteAddressMode(2000000)) return false;

        QSPI_HW_ConfigureMemoryRead(
            0xEC,
            QSPI_WIDTH_QUAD_IO,
            QSPI_ADDRLEN_32BITS,
            0, 8, 10);

        return true;
    }

    printf("QSPI: Unsupported flash manufacturer 0x%02X\r\n", mfr);
    return false;
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

    /* ---- QSPI peripheral state ---- */
    printf("QSPI Peripheral : %s\r\n",
        (QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) ? "ENABLED" : "DISABLED");

    printf("QSPI Mode       : %s\r\n",
        (QSPI_REGS->QSPI_CTRLB & QSPI_CTRLB_MODE_MEMORY) ? "MEMORY (XIP)" : "SPI/REG");

    /* ---- BAUD decode ---- */
    uint32_t baud_field =
        (QSPI_REGS->QSPI_BAUD & QSPI_BAUD_BAUD_Msk) >> QSPI_BAUD_BAUD_Pos;

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

    /* ---- Instruction frame ---- */
    uint32_t instr = QSPI_REGS->QSPI_INSTRCTRL;
    uint32_t frame = QSPI_REGS->QSPI_INSTRFRAME;

    uint8_t opcode  = (uint8_t)((instr & QSPI_INSTRCTRL_INSTR_Msk)
                                 >> QSPI_INSTRCTRL_INSTR_Pos);
    uint8_t optcode = (uint8_t)((instr & QSPI_INSTRCTRL_OPTCODE_Msk)
                                 >> QSPI_INSTRCTRL_OPTCODE_Pos);

    uint32_t width   = (frame & QSPI_INSTRFRAME_WIDTH_Msk)
                        >> QSPI_INSTRFRAME_WIDTH_Pos;
    uint32_t tfr     = (frame & QSPI_INSTRFRAME_TFRTYPE_Msk)
                        >> QSPI_INSTRFRAME_TFRTYPE_Pos;
    uint32_t addrlen = (frame & QSPI_INSTRFRAME_ADDRLEN_Msk)
                        >> QSPI_INSTRFRAME_ADDRLEN_Pos;
    uint32_t dummy   = (frame & QSPI_INSTRFRAME_DUMMYLEN_Msk)
                        >> QSPI_INSTRFRAME_DUMMYLEN_Pos;
    uint32_t optlen  = (frame & QSPI_INSTRFRAME_OPTCODELEN_Msk)
                        >> QSPI_INSTRFRAME_OPTCODELEN_Pos;

    printf("Mem Opcode      : 0x%02X\r\n", opcode);
    printf("Mem Width       : %s\r\n", qspi_width_str(width));
    printf("Mem TfrType     : %s\r\n", qspi_tfrtype_str(tfr));
    printf("Mem AddrLen     : %s\r\n", (addrlen == 3U) ? "32-bit" : "24-bit/other");
    printf("Mem OptCode     : 0x%02X (len=%lu)\r\n", optcode, optlen);
    printf("Mem Dummy       : %lu\r\n", dummy);

    /* ---- JEDEC ID ---- */
    uint32_t jedec = 0U;
    uint8_t mfr = 0xFFU;

    /* Use SINGLE_BIT read ? always safe */
    if (QSPI_HW_Read(0x9F, QSPI_WIDTH_SINGLE_BIT_SPI, &jedec, 3))
    {
        uint8_t type = (uint8_t)((jedec >> 8) & 0xFFU);
        uint8_t cap  = (uint8_t)((jedec >> 16) & 0xFFU);
        mfr = (uint8_t)(jedec & 0xFFU);

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
    if (mfr == 0x20U)
    {
        /* -------- N25Q -------- */
        uint8_t sr = N25Q_ReadStatus();
        printf("SR              : 0x%02X\r\n", sr);
        printf("Flash Ready     : %s\r\n",
               (sr & N25Q_SR_WIP_Msk) ? "NO (WIP=1)" : "YES");
        printf("Write Enable    : %s\r\n",
               (sr & N25Q_SR_WEL_Msk) ? "YES (WEL=1)" : "NO (WEL=0)");

        /* N25Q config regs */
        uint16_t nvcr;
        uint8_t vcr, evcr;

        if (N25Q_ReadNVCR(&nvcr))
            printf("NVCR            : 0x%04X\r\n", nvcr);
        else
            printf("NVCR            : READ FAILED\r\n");

        if (N25Q_ReadVCR(&vcr))
        {
            printf("VCR             : 0x%02X\r\n", vcr);
            printf("Dummy Cycles    : %u\r\n",
                (unsigned)((vcr & N25Q_VCR_DUMMY_CYCLES_Msk)
                           >> N25Q_VCR_DUMMY_CYCLES_Pos));
        }
        else
            printf("VCR             : READ FAILED\r\n");

        if (N25Q_ReadEVCR(&evcr))
        {
            printf("EVCR            : 0x%02X\r\n", evcr);
            printf("Quad I/O        : %s\r\n",
                (evcr & N25Q_EVCR_QUAD_DISABLE_Msk)
                    ? "DISABLED" : "ENABLED");
        }
        else
        {
            printf("EVCR            : READ FAILED\r\n");
            printf("Quad I/O        : UNKNOWN\r\n");
        }
    }
    else if (mfr == 0xBFU)
    {
        /* -------- SST26 -------- */
        uint8_t sr;
        if (SST26_ReadStatus(&sr))
        {
            printf("SR              : 0x%02X\r\n", sr);
            printf("Flash Ready     : %s\r\n",
                   (sr & SST26_SR_WIP_Msk) ? "NO (WIP=1)" : "YES");
        }
        else
        {
            printf("SR              : READ FAILED\r\n");
        }

        printf("Write Enable    : (managed internally)\r\n");
        printf("Quad I/O        : ENABLED (CMD 0x38)\r\n");
    }

    printf("===============================================\r\n");
}


