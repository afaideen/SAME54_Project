//
// Created by han on 25-Dec-25.
//
#include <stdio.h>
#include <string.h>
#include "../../uart_dma.h"
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

    /* 0x0B = 1-1-1 High Speed Read, 8 dummy cycles (1 dummy byte) */
    return QSPI_HW_MemoryRead_Simple(
        SST26_CMD_HIGH_SPEED_READ,
        QSPI_WIDTH_SINGLE_BIT_SPI,
        8U,
        address,
        rx,
        (size_t)len
    );
}


/* deterministic byte pattern based on absolute flash address */
static void fill_pattern(uint8_t *buf, uint32_t len, uint32_t abs_addr)
{
    /* Mix address bits so nearby pages differ clearly */
    uint32_t x = abs_addr ^ (abs_addr >> 7) ^ (abs_addr >> 13) ^ 0xA5C3F123UL;

    for (uint32_t i = 0; i < len; i++)
    {
        x = (x * 1664525UL) + 1013904223UL;   /* LCG */
        buf[i] = (uint8_t)(x >> 24);
    }
}

sst26_fulltest_result_t SST26_FullChip_Test(uint32_t base_addr, uint32_t size_bytes)
{
    if ((base_addr != 0U) || ((base_addr & (SST26_SECTOR_SIZE - 1U)) != 0U))
        return SST26_FT_ERR_ALIGN;

    if ((size_bytes == 0U) || ((size_bytes & (SST26_SECTOR_SIZE - 1U)) != 0U))
        return SST26_FT_ERR_ALIGN;

    printf("\r\n[SST26] FULL CHIP TEST (DESTRUCTIVE)\r\n");
    printf("[SST26] base=0x%08lX size=%lu bytes\r\n",
        (unsigned long)base_addr, (unsigned long)size_bytes);

    /* No JEDEC read (per your requirement). */
    if (!SST26_Reset())
        return SST26_FT_ERR_RESET;

    if (!SST26_EnableQuadIO())
        return SST26_FT_ERR_QUAD;

    if (!SST26_UnlockGlobal())
        return SST26_FT_ERR_UNLOCK;

    if (!SST26_WaitWhileBusy(SST26_FT_READY_LOOPS))
        return SST26_FT_ERR_TIMEOUT;

    const uint32_t total_sectors = size_bytes / SST26_SECTOR_SIZE;

    uint8_t tx[SST26_PAGE_SIZE];
    uint8_t rb[SST26_PAGE_SIZE];

    for (uint32_t s = 0; s < total_sectors; s++)
    {
        const uint32_t sector_addr = base_addr + (s * SST26_SECTOR_SIZE);

        /* Progress every 64 sectors (~256KB) */
        if ((s % 64U) == 0U)
        {
            uint32_t pct = (uint32_t)((100ULL * (unsigned long long)s) /
                                      (unsigned long long)total_sectors);
            printf("[SST26] Progress: %lu/%lu sectors (%lu%%) @0x%06lX\r\n",
                (unsigned long)s,
                (unsigned long)total_sectors,
                (unsigned long)pct,
                (unsigned long)sector_addr);
        }

        /* 1) Erase sector */
        if (!SST26_SectorErase(sector_addr))
        {
            printf("[SST26] Erase CMD FAIL @0x%06lX\r\n", (unsigned long)sector_addr);
            return SST26_FT_ERR_ERASE_CMD;
        }

        if (!SST26_WaitWhileBusy(SST26_FT_SECTOR_ERASE_LOOPS))
        {
            printf("[SST26] Erase TIMEOUT @0x%06lX\r\n", (unsigned long)sector_addr);
            return SST26_FT_ERR_TIMEOUT;
        }

        /* 2) Verify erased (all 0xFF) */
        for (uint32_t off = 0; off < SST26_SECTOR_SIZE; off += SST26_PAGE_SIZE)
        {
            const uint32_t addr = sector_addr + off;

            if (!SST26_HighSpeedRead(rb, SST26_PAGE_SIZE, addr))
                return SST26_FT_ERR_READBACK;

            for (uint32_t i = 0; i < SST26_PAGE_SIZE; i++)
            {
                if (rb[i] != 0xFFU)
                {
                    printf("[SST26] Erase VERIFY FAIL @0x%06lX +%lu = 0x%02X\r\n",
                        (unsigned long)addr,
                        (unsigned long)i,
                        rb[i]);
                    return SST26_FT_ERR_ERASE_VERIFY;
                }
            }
        }

        /* 3) Program sector page-by-page */
        for (uint32_t off = 0; off < SST26_SECTOR_SIZE; off += SST26_PAGE_SIZE)
        {
            const uint32_t addr = sector_addr + off;

            fill_pattern(tx, SST26_PAGE_SIZE, addr);

            if (!SST26_PageProgram(tx, SST26_PAGE_SIZE, addr))
            {
                printf("[SST26] Program CMD FAIL @0x%06lX\r\n", (unsigned long)addr);
                return SST26_FT_ERR_PROG_CMD;
            }

            if (!SST26_WaitWhileBusy(SST26_FT_PAGE_PROG_LOOPS))
            {
                printf("[SST26] Program TIMEOUT @0x%06lX\r\n", (unsigned long)addr);
                return SST26_FT_ERR_TIMEOUT;
            }
        }

        /* 4) Readback verify */
        for (uint32_t off = 0; off < SST26_SECTOR_SIZE; off += SST26_PAGE_SIZE)
        {
            const uint32_t addr = sector_addr + off;

            fill_pattern(tx, SST26_PAGE_SIZE, addr);

            if (!SST26_HighSpeedRead(rb, SST26_PAGE_SIZE, addr))
                return SST26_FT_ERR_READBACK;

            if (memcmp(tx, rb, SST26_PAGE_SIZE) != 0)
            {
                uint32_t bad_i = 0;
                for (; bad_i < SST26_PAGE_SIZE; bad_i++)
                {
                    if (tx[bad_i] != rb[bad_i])
                        break;
                }

                printf("[SST26] VERIFY FAIL @0x%06lX +%lu exp=0x%02X got=0x%02X\r\n",
                    (unsigned long)addr,
                    (unsigned long)bad_i,
                    tx[bad_i],
                    rb[bad_i]);

                return SST26_FT_ERR_VERIFY;
            }
        }
    }

    printf("[SST26] FULL CHIP TEST PASS\r\n");
    return SST26_FT_OK;
}





