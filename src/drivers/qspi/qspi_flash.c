
#include <stdio.h>
#include <string.h>
#include "../../common/board.h"
#include "qspi_flash.h"
#include "qspi_hw.h"

#if APP_USE_SST26_FLASH
#include "sst26/sst26.h"
#elif APP_USE_N25Q_FLASH
#include "n25q/n25q256a.h"
#endif

uint8_t g_qspi_jedec_id[3];
bool    g_qspi_jedec_valid;

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
    g_qspi_jedec_id[0] = (uint8_t)((jedec >> 0)  & 0xFF);   // 0xBF
    g_qspi_jedec_id[1] = (uint8_t)((jedec >> 8)  & 0xFF);   // 0x26
    g_qspi_jedec_id[2] = (uint8_t)((jedec >> 16) & 0xFF);   // 0x43

    printf("QSPI: JEDEC ID = 0x%06lX\r\n", (unsigned long)(jedec & 0x00FFFFFFUL));

    if (jedec != SST26VF064B_JEDEC_ID)
    {
		g_qspi_jedec_valid = false;
        printf("QSPI: JEDEC mismatch. expected=0x%06lX\r\n",
               (unsigned long)(SST26VF064B_JEDEC_ID & 0x00FFFFFFUL));
        return false;
    }
	g_qspi_jedec_valid = true;
  

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


    /* JEDEC read is SPI 4-4-4 by definition */
    printf("QSPI Mode       : %s\r\n",
        (QSPI_REGS->QSPI_CTRLB & QSPI_CTRLB_MODE_MEMORY) ? "MEMORY" : "SPI/REG");

    /* ---- QSPI BAUD decode (field is at bit[15:8]) ---- */
	uint32_t f_qspi = (uint32_t)CPU_CLOCK_HZ;
    uint32_t baud_field = (QSPI_REGS->QSPI_BAUD & QSPI_BAUD_BAUD_Msk) >> QSPI_BAUD_BAUD_Pos;
	uint32_t f_sck = f_qspi / (2UL * (baud_field + 1UL));
    printf("QSPI BAUD       : BAUD=%lu  (~%lu.%03lu MHz)\r\n",
        baud_field,
        f_sck / 1000000UL,
        (f_sck % 1000000UL) / 1000UL);
	uint32_t instr = QSPI_REGS->QSPI_INSTRCTRL;
	uint8_t opcode  = (uint8_t)((instr & QSPI_INSTRCTRL_INSTR_Msk) >> QSPI_INSTRCTRL_INSTR_Pos);
    uint32_t frame = QSPI_REGS->QSPI_INSTRFRAME;
	uint32_t width  = (frame & QSPI_INSTRFRAME_WIDTH_Msk) >> QSPI_INSTRFRAME_WIDTH_Pos;
	printf("Mem Opcode      : 0x%02X\r\n", opcode);
    printf("Mem Width       : %s\r\n", qspi_width_str(width));

    /* Print previously captured JEDEC */
    printf("JEDEC ID (SPI 0x9F): 0x%02X %02X %02X\r\n",
          g_qspi_jedec_id[0],
          g_qspi_jedec_id[1],
          g_qspi_jedec_id[2]);

    printf("Flash Detected  : %s\r\n",
          g_qspi_jedec_valid ? "VALID" : "INVALID");

    printf("=================================================\r\n");
}


