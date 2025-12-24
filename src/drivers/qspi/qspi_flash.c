/* qspi_flash.c: High-level QSPI flash initialization and API */
#include "qspi_hw.h"
#include "n25q256a.h"

bool QSPI_Flash_Init(void)
{
    uint32_t jedec;

    QSPI_HW_Init();
    QSPI_HW_Enable();

    if (!N25Q_Reset())
        return false;

    if (!N25Q_ReadJEDEC(&jedec))
        return false;

    if ((jedec & 0xFF) != 0x20) /* Micron manufacturer check */
        return false;

    if (!N25Q_EnableQuad())
        return false;

    if (!N25Q_SetDummyCycles(8))
        return false;

    if (!N25Q_WaitReady(500))
        return false;

    QSPI_HW_EnableXIP();

    return true;
}