/* n25q256a.c: Micron N25Q256A command helpers
 *
 * This module issues flash commands using the SAME54 QSPI peripheral helper
 * layer (qspi_hw.*). All commands below are based on the N25Q256A datasheet.
 */

#include "n25q256a.h"

/* Small de-select delay. The N25Q reset sequence requires minimum tSHSL
 * times between commands; a short NOP loop is well above nanosecond-scale mins.
 */
static inline void n25q_small_delay(void)
{
    for (volatile uint32_t i = 0; i < 5000U; i++)
    {
        __NOP();
    }
}

bool N25Q_Reset(void)
{
    if (!QSPI_HW_Command(N25Q_CMD_RESET_ENABLE, QSPI_WIDTH_SINGLE_BIT_SPI))
    {
        return false;
    }
    n25q_small_delay();
    if (!QSPI_HW_Command(N25Q_CMD_RESET_MEMORY, QSPI_WIDTH_SINGLE_BIT_SPI))
    {
        return false;
    }
    n25q_small_delay();
    return true;
}

uint32_t N25Q_ReadJEDEC(void)
{
    uint8_t id[3] = {0, 0, 0};

    /* READ ID (JEDEC) returns Manufacturer, Memory Type, Capacity */
    (void)QSPI_HW_Read(N25Q_CMD_READ_ID, QSPI_WIDTH_SINGLE_BIT_SPI, id, sizeof(id));
    return ((uint32_t)id[0]) | ((uint32_t)id[1] << 8) | ((uint32_t)id[2] << 16);
}

uint8_t N25Q_ReadStatus(void)
{
    uint8_t sr = 0U;
    (void)QSPI_HW_Read(N25Q_CMD_READ_STATUS, QSPI_WIDTH_SINGLE_BIT_SPI, &sr, 1U);
    return sr;
}

bool N25Q_WriteEnable(void)
{
    /* WREN */
    if (!QSPI_HW_Command(N25Q_CMD_WRITE_ENABLE, QSPI_WIDTH_SINGLE_BIT_SPI))
    {
        return false;
    }

    /* Verify WEL (Status Register bit1) becomes 1 */
    uint32_t guard = 100000U;
    while (guard-- != 0U)
    {
        if ((N25Q_ReadStatus() & N25Q_SR_WEL_Msk) != 0U)
        {
            return true;
        }
    }
    return false;
}

bool N25Q_WaitWhileBusy(uint32_t timeout_loops)
{
    while (timeout_loops-- != 0U)
    {
        if ((N25Q_ReadStatus() & N25Q_SR_WIP_Msk) == 0U)
        {
            return true;
        }
    }
    return false;
}

bool N25Q_ReadVCR(uint8_t *vcr)
{
    if (vcr == NULL)
    {
        return false;
    }
    return QSPI_HW_Read(N25Q_CMD_READ_VCR, QSPI_WIDTH_SINGLE_BIT_SPI, vcr, 1U);
}

bool N25Q_WriteVCR(uint8_t vcr)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }
    if (!QSPI_HW_Write(N25Q_CMD_WRITE_VCR, QSPI_WIDTH_SINGLE_BIT_SPI, &vcr, 1U))
    {
        return false;
    }
    return N25Q_WaitWhileBusy(500000U);
}

bool N25Q_SetDummyCycles(uint8_t dummy_cycles)
{
    /* VCR bits[7:4] encode the number of dummy clock cycles (0 and 15 map to 15). */
    if (dummy_cycles == 0U)
    {
        dummy_cycles = 15U;
    }
    if (dummy_cycles > 15U)
    {
        return false;
    }

    uint8_t vcr;
    if (!N25Q_ReadVCR(&vcr))
    {
        return false;
    }
    vcr = (uint8_t)((vcr & ~N25Q_VCR_DUMMY_Msk) | ((dummy_cycles << 4) & N25Q_VCR_DUMMY_Msk));
    return N25Q_WriteVCR(vcr);
}

bool N25Q_ReadEVCR(uint8_t *evcr)
{
    if (evcr == NULL)
    {
        return false;
    }
    return QSPI_HW_Read(N25Q_CMD_READ_EVCR, QSPI_WIDTH_SINGLE_BIT_SPI, evcr, 1U);
}

bool N25Q_WriteEVCR(uint8_t evcr)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }
    if (!QSPI_HW_Write(N25Q_CMD_WRITE_EVCR, QSPI_WIDTH_SINGLE_BIT_SPI, &evcr, 1U))
    {
        return false;
    }
    return N25Q_WaitWhileBusy(500000U);
}

bool N25Q_EnableQuadIO(void)
{
    /* Enable quad I/O in Enhanced Volatile Configuration Register:
     *  - Bit7 = 0 => Quad I/O protocol enabled
     *  - Bit6 = 1 => Dual I/O protocol disabled
     */
    uint8_t evcr;
    if (!N25Q_ReadEVCR(&evcr))
    {
        return false;
    }

    evcr = (uint8_t)(evcr & (uint8_t)~N25Q_EVCR_QUAD_DISABLE_Msk);
    evcr = (uint8_t)(evcr | N25Q_EVCR_DUAL_DISABLE_Msk);
    return N25Q_WriteEVCR(evcr);
}

bool N25Q_Enter4ByteAddressMode(void)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }
    if (!QSPI_HW_Command(N25Q_CMD_ENTER_4BYTE_ADDR, QSPI_WIDTH_SINGLE_BIT_SPI))
    {
        return false;
    }
    n25q_small_delay();
    return true;
}

bool N25Q_Exit4ByteAddressMode(void)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }
    if (!QSPI_HW_Command(N25Q_CMD_EXIT_4BYTE_ADDR, QSPI_WIDTH_SINGLE_BIT_SPI))
    {
        return false;
    }
    n25q_small_delay();
    return true;
}
