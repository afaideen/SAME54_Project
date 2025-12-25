/* n25q256a.h: Micron N25Q256A command helpers */

#ifndef N25Q256A_H
#define N25Q256A_H

#include <stdint.h>
#include <stdbool.h>

#include "../qspi_hw.h"

/* --- Core commands --- */
#define N25Q_CMD_WRITE_ENABLE             (0x06U)
#define N25Q_CMD_WRITE_DISABLE            (0x04U)
#define N25Q_CMD_READ_STATUS_REGISTER     (0x05U)

/* --- Reset sequence --- */
#define N25Q_CMD_RESET_ENABLE             (0x66U)
#define N25Q_CMD_RESET_MEMORY             (0x99U)

/* --- Identification --- */
#define N25Q_CMD_READ_ID                  (0x9FU) /* JEDEC: mfr + memtype + capacity */
#define N25Q_CMD_MULTIPLE_IO_READ_ID      (0xAFU) /* Harmony app_n25q uses this after quad enable */

/* --- Configuration register commands (datasheet) --- */
#define N25Q_CMD_READ_NONVOLATILE_CFG     (0xB5U) /* RDNVCR: returns 2 bytes, LSB first */
#define N25Q_CMD_WRITE_NONVOLATILE_CFG    (0xB1U) /* WRNVCR: write 2 bytes, LSB first */

#define N25Q_CMD_READ_VOLATILE_CFG        (0x85U) /* RDVCR: returns 1 byte */
#define N25Q_CMD_WRITE_VOLATILE_CFG       (0x81U) /* WRVCR: write 1 byte */

#define N25Q_CMD_READ_ENH_VOLATILE_CFG    (0x65U) /* RDVECR: returns 1 byte */
#define N25Q_CMD_WRITE_ENH_VOLATILE_CFG   (0x61U) /* WRVECR: write 1 byte */

/* --- Address mode --- */
#define N25Q_CMD_ENTER_4BYTE_ADDR_MODE    (0xB7U)
#define N25Q_CMD_EXIT_4BYTE_ADDR_MODE     (0xE9U)

/* --- Status bits --- */
#define N25Q_SR_WIP_Msk                   (1U << 0) /* Write-In-Progress */
#define N25Q_SR_WEL_Msk                   (1U << 1) /* Write Enable Latch */

/* --- VCR bitfields (Volatile Configuration Register) --- */
#define N25Q_VCR_DUMMY_CYCLES_Pos         (4U)
#define N25Q_VCR_DUMMY_CYCLES_Msk         (0xFU << N25Q_VCR_DUMMY_CYCLES_Pos)

/* --- EVCR bitfields (Enhanced Volatile Configuration Register) --- */
#define N25Q_EVCR_QUAD_DISABLE_Msk        (1U << 7)
#define N25Q_EVCR_DUAL_DISABLE_Msk        (1U << 6)


/* Public API */
bool N25Q_Reset(void);
bool N25Q_ReadJEDEC(uint32_t *jedec_out);

bool N25Q_WriteEnable(void);
uint8_t N25Q_ReadStatus(void);
bool N25Q_WaitWhileBusy(uint32_t timeout_loops);

bool N25Q_ReadNVCR(uint16_t *nvcr_out);
bool N25Q_WriteNVCR(uint16_t nvcr, uint32_t timeout_loops);

bool N25Q_ReadVCR(uint8_t *vcr_out);
bool N25Q_WriteVCR(uint8_t vcr, uint32_t timeout_loops);
bool N25Q_SetDummyCycles(uint8_t dummy_cycles, uint32_t timeout_loops);

bool N25Q_ReadEVCR(uint8_t *evcr_out);
bool N25Q_WriteEVCR(uint8_t evcr, uint32_t timeout_loops);
bool N25Q_EnableQuadIO(uint32_t timeout_loops);

bool N25Q_Enter4ByteAddressMode(uint32_t timeout_loops);
bool N25Q_Exit4ByteAddressMode(uint32_t timeout_loops);

#endif /* N25Q256A_H */
