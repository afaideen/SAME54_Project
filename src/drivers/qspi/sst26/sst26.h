//
// Created by han on 25-Dec-25.
//


#ifndef SST26_H
#define SST26_H

#include <stdint.h>
#include <stdbool.h>
#include "../qspi_hw.h"

/* Harmony app_sst26.h command set */
#define SST26_CMD_FLASH_RESET_ENABLE     (0x66U)
#define SST26_CMD_FLASH_RESET            (0x99U)

#define SST26_CMD_ENABLE_QUAD_IO         (0x38U)
#define SST26_CMD_RESET_QUAD_IO          (0xFFU)

//#define SST26_CMD_JEDEC_ID_READ          (0x9FU)
#define SST26_CMD_QUAD_JEDEC_ID_READ     (0xAFU)

#define SST26_CMD_HIGH_SPEED_READ        (0x0BU)
#define SST26_CMD_WRITE_ENABLE           (0x06U)
#define SST26_CMD_PAGE_PROGRAM           (0x02U)

#define SST26_CMD_READ_STATUS_REG        (0x05U)

#define SST26_CMD_SECTOR_ERASE           (0x20U)
#define SST26_CMD_BULK_ERASE_64K         (0xD8U)
#define SST26_CMD_CHIP_ERASE             (0xC7U)

#define SST26_CMD_UNPROTECT_GLOBAL       (0x98U)

/* Status register: WIP bit0 (Harmony uses this) */
#define SST26_SR_WIP_Msk                 (1U << 0)

/* Device ID used by Harmony demo */
#define SST26VF064B_JEDEC_ID             (0x004326BFUL)

#define LOG(...)                 (void)UART2_DMA_Log(__VA_ARGS__)

#define SST26_SECTOR_SIZE        (4096U)
#define SST26_PAGE_SIZE          (256U)
/* Busy polling loop budgets (tuned to be conservative).
   These are LOOP counts (not ms), matching SST26_WaitWhileBusy(). */
#define SST26_FT_READY_LOOPS           (2000000UL)
#define SST26_FT_PAGE_PROG_LOOPS       (3000000UL)
#define SST26_FT_SECTOR_ERASE_LOOPS    (30000000UL)


typedef enum
{
    SST26_FT_OK = 0,
    SST26_FT_ERR_ALIGN,
    SST26_FT_ERR_RESET,
    SST26_FT_ERR_QUAD,
    SST26_FT_ERR_UNLOCK,
    SST26_FT_ERR_STATUS,
    SST26_FT_ERR_TIMEOUT,
    SST26_FT_ERR_ERASE_CMD,
    SST26_FT_ERR_ERASE_VERIFY,
    SST26_FT_ERR_PROG_CMD,
    SST26_FT_ERR_READBACK,
    SST26_FT_ERR_VERIFY,
} sst26_fulltest_result_t;

/* API (mirrors Harmony behavior but keeps your naming style) */
bool SST26_Reset(void);
bool SST26_EnableQuadIO(void);
bool SST26_WriteEnable(void);
bool SST26_UnlockGlobal(void);

bool SST26_ReadJEDEC(uint32_t *jedec_out);

bool SST26_ReadStatus(uint8_t *sr_out);
bool SST26_WaitWhileBusy(uint32_t timeout_loops);

bool SST26_SectorErase(uint32_t address);
bool SST26_PageProgram(const void *tx, uint32_t len, uint32_t address);
bool SST26_HighSpeedRead(void *rx, uint32_t len, uint32_t address);
sst26_fulltest_result_t SST26_FullChip_Test(uint32_t base_addr, uint32_t size_bytes);

#endif /* SST26_H */


