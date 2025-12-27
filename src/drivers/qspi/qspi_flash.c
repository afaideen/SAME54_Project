
#include <stdio.h>
#include <string.h>
#include "../../common/board.h"
#include "../../common/systick.h"
#include "qspi_flash.h"
#include "qspi_hw.h"

#if APP_USE_SST26_FLASH
#include "sst26/sst26.h"
#elif APP_USE_N25Q_FLASH
#include "n25q/n25q256a.h"
#endif

uint8_t g_qspi_jedec_id[3];
bool    g_qspi_jedec_valid;

// --------- CRC32 (simple, tableless) ----------
static uint32_t crc32_ieee(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFUL;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= p[i];
        for (int b = 0; b < 8; b++)
        {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}
#if QSPI_FLASH_TIMELOG
static inline void qspi_timelog_print(const char *op, bool ok, uint32_t t_start_ms,
                                      uint32_t addr, uint32_t len_hint)
{
    uint32_t dt = (uint32_t)(millis() - t_start_ms); // wrap-safe

#if QSPI_FLASH_TIMELOG_FLOAT
    printf("[QSPI_FLASH] %s QSPI %s in %lu ms (%.2f s) addr=0x%08lX len=%lu\r\n",
           op,
           ok ? "PASS" : "FAIL",
           (unsigned long)dt,
           (double)dt / 1000.0,
           (unsigned long)addr,
           (unsigned long)len_hint);
#else
    printf("[QSPI_FLASH] %s QSPI %s in %lu ms (%lu.%03lu s) addr=0x%08lX len=%lu\r\n",
           op,
           ok ? "PASS" : "FAIL",
           (unsigned long)dt,
           (unsigned long)(dt / 1000UL),
           (unsigned long)(dt % 1000UL),
           (unsigned long)addr,
           (unsigned long)len_hint);
#endif
}
#endif
/* Read flash in small chunks to avoid big stack/heap */
static bool flash_read_chunked(uint32_t addr, void *dst, uint32_t len)
{
    uint8_t *out = (uint8_t *)dst;
    uint8_t rb[64];

    while (len)
    {
        uint32_t n = (len > sizeof(rb)) ? (uint32_t)sizeof(rb) : len;

        if (!SST26_HighSpeedRead(rb, n, addr))
            return false;

        memcpy(out, rb, n);

        addr += n;
        out  += n;
        len  -= n;
    }
    return true;
}

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
//    QSPI_HW_Initialize();

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

static bool flash_write_pages(uint32_t addr, const uint8_t *src, uint32_t len)
{
    // Programs must not cross SST26_PAGE_SIZE boundaries.
    while (len)
    {
        uint32_t page_off = addr % SST26_PAGE_SIZE;
        uint32_t chunk = SST26_PAGE_SIZE - page_off;
        if (chunk > len) chunk = len;

        // IMPORTANT: SST26_PageProgram() will happily accept any len,
        // but the FLASH itself will wrap if we cross page boundary.
        if (!SST26_PageProgram(src, chunk, addr))
            return false;

        // Wait for WIP to clear after each program
        if (!SST26_WaitWhileBusy(SST26_FT_PAGE_PROG_LOOPS))
            return false;

        addr += chunk;
        src  += chunk;
        len  -= chunk;
    }
    return true;
}

static bool flash_verify(uint32_t addr, const void *ref, uint32_t len)
{
    // Small-buffer verify to avoid big stack usage
    uint8_t rb[64];
    const uint8_t *exp = (const uint8_t *)ref;

    while (len)
    {
        uint32_t n = (len > sizeof(rb)) ? sizeof(rb) : len;

        if (!SST26_HighSpeedRead(rb, n, addr))
            return false;

        if (memcmp(rb, exp, n) != 0)
            return false;

        addr += n;
        exp  += n;
        len  -= n;
    }
    return true;
}

/*
 * QSPI_Flash_ReadAddr()
 * --------------------
 * Reads [header][payload] stored by QSPI_Flash_WriteAddr().
 *
 * address:       flash offset where the object starts
 * obj_out:       destination buffer for payload
 * obj_max_len:   size of destination buffer
 * hdr_out:       optional pointer to receive header (can be NULL)
 * verify_crc:    true = verify payload_crc before returning success
 *
 * Returns true only if header is valid AND payload fits AND (optionally) CRC is valid.
 */
bool QSPI_Flash_ReadAddr(uint32_t address,
                         void *obj_out, uint32_t obj_max_len,
                         qspi_obj_hdr_t *hdr_out,
                         bool verify_crc)
{
    if ((obj_out == NULL) || (obj_max_len == 0U))
        return false;

    // 1) Read header
    qspi_obj_hdr_t hdr;
    if (!flash_read_chunked(address, &hdr, (uint32_t)sizeof(hdr)))
        return false;

    // 2) Detect "empty / erased" region quickly (common in flash)
    //    If sector is erased, bytes are 0xFF, so magic will be 0xFFFFFFFF.
    if (hdr.magic == 0xFFFFFFFFUL)
        return false;

    // 3) Validate header basics
    if (hdr.magic != QSPI_OBJ_MAGIC)
        return false;

    if (hdr.header_len != (uint16_t)sizeof(qspi_obj_hdr_t))
        return false;

    if (hdr.payload_len == 0U)
        return false;

    if (hdr.payload_len > obj_max_len)
        return false;  // caller buffer too small

    // 4) Validate header CRC (treat header_crc as 0 when computing)
    uint32_t saved_hcrc = hdr.header_crc;
    hdr.header_crc = 0U;
    uint32_t calc_hcrc = crc32_ieee(&hdr, sizeof(hdr));
    hdr.header_crc = saved_hcrc;

    if (calc_hcrc != saved_hcrc)
        return false;

    // 5) Read payload
    uint32_t payload_addr = address + (uint32_t)sizeof(qspi_obj_hdr_t);
    if (!flash_read_chunked(payload_addr, obj_out, hdr.payload_len))
        return false;

    // 6) Optional payload CRC verify
    if (verify_crc)
    {
        uint32_t calc_pcrc = crc32_ieee(obj_out, hdr.payload_len);
        if (calc_pcrc != hdr.payload_crc)
            return false;
    }

    // 7) Return header to caller if requested
    if (hdr_out != NULL)
        *hdr_out = hdr;

    return true;
}

bool QSPI_Flash_ReadSector(int sector,
                           void *obj_out, uint32_t obj_max_len,
                           qspi_obj_hdr_t *hdr_out,
                           bool verify_crc)
{
    if (sector < 0)
        return false;

    if ((uint32_t)sector >= QSPI_OBJ_MAX_SECTORS)
        return false;

    uint32_t addr = QSPI_OBJ_STORE_BASE + ((uint32_t)sector * SST26_SECTOR_SIZE);
    return QSPI_Flash_ReadAddr(addr, obj_out, obj_max_len, hdr_out, verify_crc);
}

/*
 * QSPI_Flash_WriteAddr()
 * ---------------------
 * Writes: [header][payload] into flash starting at 'address'.
 *
 * This function is "destructive" to the sectors it touches:
 * it will ERASE required sectors first, then program.
 *
 * address: flash offset (0..flash_size-1)
 */
bool QSPI_Flash_WriteAddr(uint32_t address,
                          const void *obj, uint32_t obj_len,
                          uint32_t type_id, uint32_t version)
{
#if QSPI_FLASH_TIMELOG
    uint32_t t_start_ms = millis();
#endif
    bool ok = false;

    if ((obj == NULL) || (obj_len == 0U))
        goto out;

    // 1) Build header in RAM
    qspi_obj_hdr_t hdr;
    hdr.magic       = QSPI_OBJ_MAGIC;
    hdr.header_len  = (uint16_t)sizeof(hdr);
    hdr.flags       = 0U;
    hdr.type_id     = type_id;
    hdr.version     = version;
    hdr.payload_len = obj_len;
    hdr.payload_crc = crc32_ieee(obj, obj_len);

    hdr.header_crc  = 0U;
    hdr.header_crc  = crc32_ieee(&hdr, sizeof(hdr));

    // 2) Compute total write size
    uint32_t total_len = (uint32_t)sizeof(hdr) + obj_len;

    // 3) ERASE all touched sectors
    uint32_t start = address & ~(SST26_SECTOR_SIZE - 1U);
    uint32_t end   = (address + total_len + (SST26_SECTOR_SIZE - 1U)) & ~(SST26_SECTOR_SIZE - 1U);

    for (uint32_t a = start; a < end; a += SST26_SECTOR_SIZE)
    {
        if (!SST26_SectorErase(a))
            goto out;

        if (!SST26_WaitWhileBusy(SST26_FT_SECTOR_ERASE_LOOPS))
            goto out;
    }

    // 4) Program header
    if (!flash_write_pages(address, (const uint8_t *)&hdr, (uint32_t)sizeof(hdr)))
        goto out;

    // 5) Program payload
    if (!flash_write_pages(address + (uint32_t)sizeof(hdr), (const uint8_t *)obj, obj_len))
        goto out;

    // 6) Verify
    if (!flash_verify(address, &hdr, (uint32_t)sizeof(hdr)))
        goto out;

    if (!flash_verify(address + (uint32_t)sizeof(hdr), obj, obj_len))
        goto out;

    ok = true;

out:
#if QSPI_FLASH_TIMELOG
    qspi_timelog_print("Write", ok, t_start_ms, address, obj_len);
#endif
    return ok;
}


/*
 * QSPI_Flash_WriteSector()
 * -----------------------
 * Convenience wrapper: sector index -> base+sector*sector_size.
 * Intended for "one object per sector" storage policy.
 */
bool QSPI_Flash_WriteSector(int sector,
                            const void *obj, uint32_t obj_len,
                            uint32_t type_id, uint32_t version)
{
    if (sector < 0)
        return false;

    if ((uint32_t)sector >= QSPI_OBJ_MAX_SECTORS)
        return false;

    uint32_t addr = QSPI_OBJ_STORE_BASE + ((uint32_t)sector * SST26_SECTOR_SIZE);
    return QSPI_Flash_WriteAddr(addr, obj, obj_len, type_id, version);
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

static const char *qspi_io_mode_str(uint32_t width)
{
    /* WIDTH encoding (your existing qspi_width_str shows):
       0: 1-1-1  -> SINGLE
       1: 1-1-2  -> DUAL
       2: 1-1-4  -> QUAD
       3: 1-2-2  -> DUAL
       4: 1-4-4  -> QUAD
       5: 2-2-2  -> DUAL
       6: 4-4-4  -> QUAD
     */
    switch (width) {
        case 0: return "SINGLE";
        case 1: case 3: case 5: return "DUAL";
        case 2: case 4: case 6: return "QUAD";
        default: return "UNKNOWN";
    }
}

void QSPI_Flash_Diag_Print(void)
{
    uint32_t f_qspi = (uint32_t)CPU_CLOCK_HZ;
    uint32_t baud_field = (QSPI_REGS->QSPI_BAUD & QSPI_BAUD_BAUD_Msk) >> QSPI_BAUD_BAUD_Pos;
	uint32_t f_sck = f_qspi / (2UL * (baud_field + 1UL));
    uint32_t instr = QSPI_REGS->QSPI_INSTRCTRL;
	uint8_t opcode  = (uint8_t)((instr & QSPI_INSTRCTRL_INSTR_Msk) >> QSPI_INSTRCTRL_INSTR_Pos);
    uint32_t frame = QSPI_REGS->QSPI_INSTRFRAME;
	uint32_t width  = (frame & QSPI_INSTRFRAME_WIDTH_Msk) >> QSPI_INSTRFRAME_WIDTH_Pos;
    
    printf("\r\n");
    printf("=============== QSPI DIAGNOSTIC ===============\r\n");

    /* ---- QSPI peripheral state (use STATUS, not CTRLA) ---- */
    printf("QSPI Peripheral : %s\r\n",
        (QSPI_REGS->QSPI_STATUS & QSPI_STATUS_ENABLE_Msk) ? "ENABLED" : "DISABLED");


    /* JEDEC read is SPI 4-4-4 by definition */
    printf("QSPI Mode       : %s\r\n",
        (QSPI_REGS->QSPI_CTRLB & QSPI_CTRLB_MODE_MEMORY) ? "MEMORY" : "SPI/REG");
    printf("QSPI I/O Mode   : %s\r\n", qspi_io_mode_str(width));

    /* ---- QSPI BAUD decode (field is at bit[15:8]) ---- */
	
    printf("QSPI BAUD       : BAUD=%lu  (~%lu.%03lu MHz)\r\n",
        baud_field,
        f_sck / 1000000UL,
        (f_sck % 1000000UL) / 1000UL);
	
	printf("Mem Opcode      : 0x%02X\r\n", opcode);
    printf("Mem Width       : %s\r\n", qspi_width_str(width));

    /* Print previously captured JEDEC */
    printf("JEDEC ID        : 0x%02X %02X %02X\r\n",
          g_qspi_jedec_id[0],
          g_qspi_jedec_id[1],
          g_qspi_jedec_id[2]);

    printf("Flash Detected  : %s\r\n",
          g_qspi_jedec_valid ? "VALID" : "INVALID");

    printf("=================================================\r\n");
}

typedef struct
{
    uint32_t device_id;
    uint32_t boot_count;
    uint16_t calib_offset;
    uint16_t calib_gain;
    uint8_t  flags;
    uint8_t  reserved[3];   // pad to 4-byte alignment
} device_cfg_t;

device_cfg_t cfg;
static void DeviceCfg_Log(const char *tag, const device_cfg_t *cfg)
{
    printf(
            "[QSPI][%s] device_cfg:\r\n"
            "  device_id   = 0x%08lX\r\n"
            "  boot_count  = %lu\r\n"
            "  calib_off   = %u\r\n"
            "  calib_gain  = %u\r\n"
            "  flags       = 0x%02X\r\n",
            tag,
            (unsigned long)cfg->device_id,
            (unsigned long)cfg->boot_count,
            (unsigned)cfg->calib_offset,
            (unsigned)cfg->calib_gain,
            (unsigned)cfg->flags
        );
}
void QSPI_FLASH_Example_WriteRead(void)
{
    #define QSPI_CFG_FLASH_ADDR   (8U * 4096U)   // sector 8
    bool ok;
    
    // Initialize data cfg
    cfg.device_id    = 0xE54A1234;
    cfg.boot_count   = 17;
    cfg.calib_offset = 102;
    cfg.calib_gain   = 4096;
    cfg.flags        = 0x01;
    DeviceCfg_Log("WRITE", &cfg);

    ok = QSPI_Flash_WriteAddr(
            QSPI_CFG_FLASH_ADDR,   // address in QSPI flash
            &cfg,                  // pointer to object
            sizeof(cfg),            // payload size
            1,                      // type_id (user-defined)
            1                       // version (schema version)
         );

    if (!ok)
    {
        printf("[QSPI_Flash] Config write FAILED\r\n");
    }
    else
    {
        printf("[QSPI_Flash] Config write OK\r\n");
    }
    
    device_cfg_t cfg_read;
    qspi_obj_hdr_t meta;

    ok = QSPI_Flash_ReadAddr(
                QSPI_CFG_FLASH_ADDR,
                &cfg_read,
                (uint32_t)sizeof(cfg_read),
                &meta,
                true   // verify payload CRC
              );

    if (!ok)
    {
        printf("[QSPI] Config read FAILED or empty\r\n");
    }
    else
    {
        printf("[QSPI] Config read OK: type=%lu ver=%lu len=%lu\r\n",
                  (unsigned long)meta.type_id,
                  (unsigned long)meta.version,
                  (unsigned long)meta.payload_len);

        printf("device_id=0x%08lX boot=%lu flags=0x%02X\r\n",
                  (unsigned long)cfg_read.device_id,
                  (unsigned long)cfg_read.boot_count,
                  (unsigned)cfg_read.flags);
        DeviceCfg_Log("READ", &cfg_read);
    }
}


