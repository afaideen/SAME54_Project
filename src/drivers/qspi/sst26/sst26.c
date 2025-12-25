//
// Created by han on 25-Dec-25.
//

#include "sst26.h"
/* Internal driver state: quad mode enabled or not */
static bool sst26_quad_enabled = false;

static inline qspi_width_t sst26_cmd_width(void)
{
    return sst26_quad_enabled ? QSPI_WIDTH_QUAD_CMD
                              : QSPI_WIDTH_SINGLE_BIT_SPI;
}

bool SST26_Reset(void)
{
    sst26_quad_enabled = false;
    if (!QSPI_HW_Command(SST26_CMD_FLASH_RESET_ENABLE, QSPI_WIDTH_SINGLE_BIT_SPI))
        return false;

    if (!QSPI_HW_Command(SST26_CMD_FLASH_RESET, QSPI_WIDTH_SINGLE_BIT_SPI))
        return false;

    return true;
}

bool SST26_EnableQuadIO(void)
{
    if (!QSPI_HW_Command(SST26_CMD_ENABLE_QUAD_IO,
                         QSPI_WIDTH_SINGLE_BIT_SPI))
        return false;

    sst26_quad_enabled = true;
    return true;
}


bool SST26_WriteEnable(void)
{
    return QSPI_HW_Command(SST26_CMD_WRITE_ENABLE,
                            sst26_cmd_width());
}


bool SST26_UnlockGlobal(void)
{
    /* Harmony: WREN then 0x98 in QUAD_CMD */
    if (!SST26_WriteEnable())
        return false;

    return QSPI_HW_Command(SST26_CMD_UNPROTECT_GLOBAL, QSPI_WIDTH_QUAD_CMD);
}

bool SST26_ReadJEDEC(uint32_t *jedec_out)
{
    if (jedec_out == NULL)
        return false;

    uint8_t id[3] = {0};

    /* Harmony: 0xAF, QUAD_CMD, dummy_cycles=2, read 3 bytes */
    if (!QSPI_HW_ReadEx(SST26_CMD_QUAD_JEDEC_ID_READ,
                        QSPI_WIDTH_QUAD_CMD,
                        2U,
                        id,
                        sizeof(id)))
    {
        return false;
    }

    *jedec_out = ((uint32_t)id[0]) | ((uint32_t)id[1] << 8) | ((uint32_t)id[2] << 16);
    return true;
}

bool SST26_ReadStatus(uint8_t *sr_out)
{
    if (sr_out == NULL)
        return false;

    /* Harmony: 0x05, QUAD_CMD, dummy_cycles=2, read 1 byte */
    return QSPI_HW_ReadEx(SST26_CMD_READ_STATUS_REG,
                          QSPI_WIDTH_QUAD_CMD,
                          2U,
                          sr_out,
                          1U);
}

bool SST26_WaitWhileBusy(uint32_t timeout_loops)
{
    while (timeout_loops-- != 0U)
    {
        uint8_t sr = 0;
        if (!SST26_ReadStatus(&sr))
            return false;

        if ((sr & SST26_SR_WIP_Msk) == 0U)
            return true;
    }
    return false;
}

bool SST26_SectorErase(uint32_t address)
{
    /* Harmony: WREN then 0x20 with address, width QUAD_CMD */
    if (!SST26_WriteEnable())
        return false;

    return QSPI_HW_CommandAddr(SST26_CMD_SECTOR_ERASE,
                               QSPI_WIDTH_QUAD_CMD,
                               QSPI_ADDRLEN_24BITS,
                               address);
}

bool SST26_PageProgram(const void *tx, uint32_t len, uint32_t address)
{
    if ((tx == NULL) || (len == 0U))
        return false;

    /* Harmony: WREN then PAGE_PROGRAM with QUAD_CMD */
    if (!SST26_WriteEnable())
        return false;

    return QSPI_HW_MemoryWrite(SST26_CMD_PAGE_PROGRAM,
                               QSPI_WIDTH_QUAD_CMD,
                               QSPI_ADDRLEN_24BITS,
                               false, 0, 0,
                               0,
                               tx, (size_t)len,
                               address);
}

bool SST26_HighSpeedRead(void *rx, uint32_t len, uint32_t address)
{
    if ((rx == NULL) || (len == 0U))
        return false;

    /* Harmony: 0x0B, QUAD_CMD, dummy_cycles=6 */
    return QSPI_HW_MemoryRead(SST26_CMD_HIGH_SPEED_READ,
                              QSPI_WIDTH_QUAD_CMD,
                              QSPI_ADDRLEN_24BITS,
                              false, 0, 0,
                              false,
                              6U,
                              rx, (size_t)len,
                              address);
}
