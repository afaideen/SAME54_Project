//
// Created by han on 25-Dec-25.
//
#include <stdio.h>
#include <string.h>
#include "../../uart_dma.h"
#include "sst26.h"
#include "../../../common/systick.h"
/* Internal driver state: quad mode enabled or not */
static bool sst26_quad_enabled = false;

static void sst26_debug_print_sr_200ms(uint8_t sr)
{
    static uint32_t last_ms = 0;
    uint32_t now = millis();

    if ((now - last_ms) >= 200U)
    {
        last_ms = now;
        printf("[SST26] SR=0x%02X\r\n", (unsigned)sr);
    }
}


static bool SST26_Wait_Ready_Ms(uint32_t timeout_ms, uint8_t *last_sr)
{
    uint32_t t0 = millis();
    while ((millis() - t0) < timeout_ms)
    {
        uint8_t sr = 0;
        if (!SST26_ReadStatus(&sr)){
            if (last_sr) *last_sr = 0xEE;
            return false;
        }

        if (last_sr) *last_sr = sr;
            
        /* print at most once every 200ms */
//        sst26_debug_print_sr_200ms(sr);
        if ((sr & SST26_SR_WIP_Msk) == 0U)
            return true;
    }
    return false;
}

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
bool SST26_ChipErase(uint32_t timeout_ms)
{
    /* Harmony APP_Erase() behavior for chip erase:
     *  - WREN (sent in QUAD_CMD in the demo)
     *  - CHIP ERASE opcode
     *  - poll SR.WIP until clear
     */
    if (timeout_ms == 0U)
    {
        timeout_ms = (uint32_t)SST26_CHIP_ERASE_TIMEOUT_MS;
    }
    uint32_t t_start_ = millis();

    /* Harmony-style: WREN -> CHIP ERASE -> poll WIP clear */
    if (!SST26_WriteEnable())
    {
        printf("[SST26] ChipErase: WREN failed\r\n");
        return false;
    }

    /* Use current command width (quad if SST26_EnableQuadIO() was called). */
    if (!QSPI_HW_Command(SST26_CMD_CHIP_ERASE, sst26_cmd_width()))
    {
        return false;
    }
    /* NEW: confirm busy actually starts (WIP becomes 1) */
    uint32_t t_start = millis();
    uint8_t sr = 0;
    while ((millis() - t_start) < 100U)
    {
        if (!SST26_ReadStatus(&sr))
        {
            printf("[SST26] ChipErase: ReadStatus failed\r\n");
            return false;
        }
        if ((sr & SST26_SR_WIP_Msk) != 0U)
        {
            break; /* started */
        }
    }

    if ((sr & SST26_SR_WIP_Msk) == 0U)
    {
        /* If we never saw WIP=1, treat as ?erase did not start? */
        printf("[SST26] ChipErase: did not start (SR=0x%02X)\r\n", (unsigned)sr);
        return false;
    }

    uint32_t t0 = millis();
    uint8_t sr_last = 0;

    bool ok = SST26_Wait_Ready_Ms(timeout_ms, &sr_last);
    
    printf("[SST26] ChipErase %s in %lu ms (%.2f s)\r\n",
           ok ? "PASS" : "FAIL",
           (unsigned long)(millis() - t_start_),
           (double)(millis() - t_start_) / 1000.0);

	if (!ok)
    {
        printf("[SST26] ChipErase TIMEOUT SR=0x%02X\r\n", (unsigned)sr_last);
        return false;
    }
    
    return true;
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

    /* Harmony: 0x0B, QUAD_CMD, dummy=6 */
    return QSPI_HW_MemoryRead_Simple(
        SST26_CMD_HIGH_SPEED_READ,
        QSPI_WIDTH_QUAD_CMD,
        6U,
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
//        printf("[SST26] Erase sector @0x%06lX\r\n", (unsigned long)sector_addr);
        /* 1) Erase sector */
        if (!SST26_SectorErase(sector_addr))
        {
            printf("[SST26] Erase CMD FAIL @0x%06lX\r\n", (unsigned long)sector_addr);
            return SST26_FT_ERR_ERASE_CMD;
        }
//        printf("[SST26] Erase CMD OK, waiting WIP clear...\r\n");

        uint8_t sr_last = 0;
        if (!SST26_Wait_Ready_Ms(3000U, &sr_last))
        {
            printf("[SST26] Erase TIMEOUT @0x%06lX SR=0x%02X\r\n",
           (unsigned long)sector_addr, (unsigned)sr_last);
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

            uint8_t sr_last = 0;
            if (!SST26_Wait_Ready_Ms(50U, &sr_last))
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

static bool is_all_ff(uint32_t addr, uint32_t len)
{
    uint8_t buf[64];
    while (len)
    {
        uint32_t n = (len > sizeof(buf)) ? sizeof(buf) : len;
        if (!SST26_HighSpeedRead(buf, n, addr)) return false;
        for (uint32_t i = 0; i < n; i++) if (buf[i] != 0xFF) return false;
        addr += n;
        len  -= n;
    }
    return true;
}

void SST26_ChipErase_Prove(void)
{
    static const char a[] = "AAAA\r\n";
    static const char b[] = "BBBB\r\n";
    const uint32_t addr0 = 0x000000;
    const uint32_t addr1 = 0x7F0000;

    printf("\r\n[SST26] Prove ChipErase\r\n");

    /* Program two far locations */
    (void)SST26_PageProgram(a, (uint32_t)sizeof(a)-1U, addr0);
    (void)SST26_WaitWhileBusy(SST26_FT_PAGE_PROG_LOOPS);

    (void)SST26_PageProgram(b, (uint32_t)sizeof(b)-1U, addr1);
    (void)SST26_WaitWhileBusy(SST26_FT_PAGE_PROG_LOOPS);

    /* Confirm not erased anymore */
    printf("[SST26] Before erase: addr0 allFF=%u, addr1 allFF=%u\r\n",
           (unsigned)is_all_ff(addr0, 64),
           (unsigned)is_all_ff(addr1, 64));

    /* Chip erase and time it */
    uint32_t t0 = millis();
    bool ok = SST26_ChipErase(0);
    uint32_t dt = millis() - t0;

    printf("[SST26] ChipErase %s in %lu ms (%.2f s)\r\n",
           ok ? "PASS" : "FAIL",
           (unsigned long)dt,
           (double)dt / 1000.0);

    /* Verify erased */
    printf("[SST26] After erase:  addr0 allFF=%u, addr1 allFF=%u\r\n",
           (unsigned)is_all_ff(addr0, 64),
           (unsigned)is_all_ff(addr1, 64));
}

void SST26_Test_WriteRead_HelloWorld(void)
{
    static const char msg[] = "Hello world\r\n";
    const uint32_t addr = 0x000000;                 // start of chip

    /* Read-before (optional, just to see what?s there after erase) */
    uint8_t pre[sizeof(msg)];
    memset(pre, 0, sizeof(pre));
    if (!SST26_HighSpeedRead(pre, (uint32_t)sizeof(msg) - 1U, addr))
    {
        printf("[SST26] Read-before FAIL\r\n");
        return;
    }
    printf("\r\n");
    printf("[SST26] Read-before @0x%06lX: ", (unsigned long)addr);
    for (uint32_t i = 0; i < (uint32_t)sizeof(msg) - 1U; i++)
        printf("%02X ", pre[i]);
    printf("\r\n");

    /* Program the string */
    if (!SST26_PageProgram(msg, (uint32_t)sizeof(msg) - 1U, addr))
    {
        printf("[SST26] PageProgram FAIL\r\n");
        return;
    }

    /* Wait for program to finish (loop-budget already defined in sst26.h) */
    if (!SST26_WaitWhileBusy(SST26_FT_PAGE_PROG_LOOPS))
    {
        uint8_t sr = 0;
        (void)SST26_ReadStatus(&sr);
        printf("[SST26] Program BUSY TIMEOUT SR=0x%02X\r\n", (unsigned)sr);
        return;
    }

    /* Read back and compare */
    uint8_t rb[sizeof(msg)];
    memset(rb, 0, sizeof(rb));

    if (!SST26_HighSpeedRead(rb, (uint32_t)sizeof(msg) - 1U, addr))
    {
        printf("[SST26] Readback FAIL\r\n");
        return;
    }

    rb[sizeof(msg) - 1U] = '\0';  // make it printable

    if (memcmp(msg, rb, sizeof(msg) - 1U) != 0)
    {
        printf("[SST26] VERIFY FAIL\r\n");
        printf("[SST26] Exp: \"%s\"\r\n", msg);
        printf("[SST26] Got: \"%s\"\r\n", (char *)rb);

        /* Hex diff (first mismatch) */
        for (uint32_t i = 0; i < (uint32_t)sizeof(msg) - 1U; i++)
        {
            if (((uint8_t)msg[i]) != rb[i])
            {
                printf("[SST26] Mismatch @+%lu exp=%02X got=%02X\r\n",
                       (unsigned long)i,
                       (unsigned)((uint8_t)msg[i]),
                       (unsigned)rb[i]);
                break;
            }
        }
        return;
    }

    printf("[SST26] WRITE+READ PASS: \"%s\"\r\n", (char *)rb);
}

void SST26_Test_ChipErase_Timing(void)
{
    printf("[SST26] CHIP ERASE (DESTRUCTIVE)\r\n");
    if (!SST26_ChipErase(0))   // 0 = use default timeout macro
    {
        printf("[SST26] Chip erase FAILED\r\n");
    }
    else
    {
        printf("[SST26] Chip erase OK\r\n");
    }
}

void SST26_Example_Erase(void)
{
    //    sst26_fulltest_result_t r = SST26_FullChip_Test(0x00000000UL, 8UL * 1024UL * 1024UL);
//    printf("\r\nFullChipTest result=%d\r\n", (int)r);

    SST26_Test_ChipErase_Timing();
    uint8_t sr=0;
    (void)SST26_ReadStatus(&sr);
    printf("[SST26] SR after ChipErase return = 0x%02X\r\n", (unsigned)sr);
//    SST26_Test_WriteRead_HelloWorld();
    SST26_ChipErase_Prove();
}





