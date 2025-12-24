/* n25q256a.c: Implementation of N25Q256A flash helper functions */
#include "n25q256a.h"
#include "qspi_hw.h"

/* Commands */
#define CMD_RESET_EN     0x66
#define CMD_RESET_MEM    0x99
#define CMD_RDID         0x9F
#define CMD_WREN         0x06
#define CMD_RDSR         0x05
#define CMD_WRNVCR       0xB1
#define CMD_RDNVCR       0xB5

#define NVCR_QE_Msk      (1u << 7)

bool N25Q_Reset(void)
{
    QSPI_Command(CMD_RESET_EN);
    QSPI_Command(CMD_RESET_MEM);
    return true;
}

bool N25Q_ReadJEDEC(uint32_t *id)
{
    uint8_t buf[3];
    QSPI_CommandRead(CMD_RDID, buf, 3);
    *id = (buf[0]) | (buf[1] << 8) | (buf[2] << 16);
    return true;
}

bool N25Q_WriteEnable(void)
{
    return QSPI_Command(CMD_WREN);
}

bool N25Q_ReadStatus(uint8_t *sr)
{
    return QSPI_CommandRead(CMD_RDSR, sr, 1);
}

bool N25Q_EnableQuad(void)
{
    uint8_t nvcr;

    QSPI_CommandRead(CMD_RDNVCR, &nvcr, 1);
    if (nvcr & NVCR_QE_Msk)
        return true;

    nvcr |= NVCR_QE_Msk;
    N25Q_WriteEnable();
    QSPI_CommandWrite(CMD_WRNVCR, &nvcr, 1);

    return true;
}

bool N25Q_SetDummyCycles(uint8_t cycles)
{
    /* simplified – device-specific tuning */
    (void)cycles;
    return true;
}

bool N25Q_WaitReady(uint32_t timeout_ms)
{
    uint8_t sr;
    while (timeout_ms--) {
        N25Q_ReadStatus(&sr);
        if ((sr & 0x01) == 0)
            return true;
    }
    return false;
}