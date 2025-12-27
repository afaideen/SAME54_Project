/* qspi_flash.h: High-level QSPI flash interface */
#ifndef QSPI_FLASH_H
#define QSPI_FLASH_H

#include <stdbool.h>
typedef struct __attribute__((packed))
{
    uint32_t magic;
    uint16_t header_len;
    uint16_t flags;
    uint32_t type_id;
    uint32_t version;
    uint32_t payload_len;
    uint32_t payload_crc;
    uint32_t header_crc;
} qspi_obj_hdr_t;

#define APP_USE_SST26_FLASH  1
#define APP_USE_N25Q_FLASH   0

#if (APP_USE_SST26_FLASH + APP_USE_N25Q_FLASH) != 1
#error "Select exactly one flash type"
#endif



// --------- user policy knobs ----------
#define QSPI_OBJ_MAGIC            (0x314A424FUL) // 'OBJ1' (example)
#define QSPI_OBJ_STORE_BASE       (0x000000UL)   // flash offset base for your objects region
#define QSPI_OBJ_MAX_SECTORS      (256U)         // cap safety (example)

#define QSPI_FLASH_TIMELOG      1
#if QSPI_FLASH_TIMELOG == 1
    #define QSPI_FLASH_TIMELOG_FLOAT    1
#endif
bool QSPI_Flash_Init(void);
bool QSPI_Flash_ReadAddr(uint32_t address,
                         void *obj_out, uint32_t obj_max_len,
                         qspi_obj_hdr_t *hdr_out,
                         bool verify_crc);
bool QSPI_Flash_ReadSector(int sector,
                           void *obj_out, uint32_t obj_max_len,
                           qspi_obj_hdr_t *hdr_out,
                           bool verify_crc);
bool QSPI_Flash_WriteAddr(uint32_t address,
                          const void *obj, uint32_t obj_len,
                          uint32_t type_id, uint32_t version);
bool QSPI_Flash_WriteSector(int sector,
                            const void *obj, uint32_t obj_len,
                            uint32_t type_id, uint32_t version);
void QSPI_Flash_Diag_Print(void);
void QSPI_FLASH_Example_WriteRead(void);

#endif /* QSPI_FLASH_H */