#include "n25q256a.h"

static bool s_use_quad_cmd = false; /* Harmony-style: after quad enable, use QUAD_CMD ops */

static inline void n25q_small_delay(void)
{
    for (volatile uint32_t i = 0; i < 5000U; i++)
    {
        __NOP();
    }
}

static inline qspi_width_t n25q_cmd_width(void)
{
    return s_use_quad_cmd ? QSPI_WIDTH_QUAD_CMD : QSPI_WIDTH_SINGLE_BIT_SPI;
}

bool N25Q_Reset(void)
{
    /* After reset, default back to 1-1-1 until quad is enabled again */
    s_use_quad_cmd = false;

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

bool N25Q_ReadJEDEC(uint32_t *jedec_out)
{
    if (jedec_out == NULL)
    {
        return false;
    }

    uint8_t id[3] = { 0U, 0U, 0U };

    /* Harmony app_n25q: after quad enable, use 0xAF with QUAD_CMD */
    if (s_use_quad_cmd)
    {
        if (QSPI_HW_ReadEx(N25Q_CMD_MULTIPLE_IO_READ_ID, QSPI_WIDTH_QUAD_CMD, 0U, id, sizeof(id)))
        {
            *jedec_out = ((uint32_t)id[0]) | ((uint32_t)id[1] << 8) | ((uint32_t)id[2] << 16);
            return true;
        }
        /* fall back to 0x9F if something is off */
    }

    if (!QSPI_HW_Read(N25Q_CMD_READ_ID, QSPI_WIDTH_SINGLE_BIT_SPI, id, sizeof(id)))
    {
        return false;
    }

    *jedec_out = ((uint32_t)id[0]) | ((uint32_t)id[1] << 8) | ((uint32_t)id[2] << 16);
    return true;
}

uint8_t N25Q_ReadStatus(void)
{
    uint8_t sr = 0U;
    (void)QSPI_HW_Read(N25Q_CMD_READ_STATUS_REGISTER, n25q_cmd_width(), &sr, 1U);
    return sr;
}

bool N25Q_WriteEnable(void)
{
    /* Harmony uses QUAD_CMD for WREN after quad enable */
    if (!QSPI_HW_Command(N25Q_CMD_WRITE_ENABLE, n25q_cmd_width()))
    {
        return false;
    }

    uint32_t guard = 200000U;
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

bool N25Q_ReadNVCR(uint16_t *nvcr_out)
{
    if (nvcr_out == NULL)
    {
        return false;
    }

    uint8_t buf[2] = { 0U, 0U };
    if (!QSPI_HW_Read(N25Q_CMD_READ_NONVOLATILE_CFG, QSPI_WIDTH_SINGLE_BIT_SPI, buf, sizeof(buf)))
    {
        return false;
    }

    *nvcr_out = (uint16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    return true;
}

bool N25Q_WriteNVCR(uint16_t nvcr, uint32_t timeout_loops)
{
    uint8_t buf[2];
    buf[0] = (uint8_t)(nvcr & 0xFFU);
    buf[1] = (uint8_t)((nvcr >> 8) & 0xFFU);

    if (!N25Q_WriteEnable())
    {
        return false;
    }

    if (!QSPI_HW_Write(N25Q_CMD_WRITE_NONVOLATILE_CFG, QSPI_WIDTH_SINGLE_BIT_SPI, buf, sizeof(buf)))
    {
        return false;
    }

    return N25Q_WaitWhileBusy(timeout_loops);
}

bool N25Q_ReadVCR(uint8_t *vcr_out)
{
    if (vcr_out == NULL)
    {
        return false;
    }
    return QSPI_HW_Read(N25Q_CMD_READ_VOLATILE_CFG, QSPI_WIDTH_SINGLE_BIT_SPI, vcr_out, 1U);
}

bool N25Q_WriteVCR(uint8_t vcr, uint32_t timeout_loops)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }

    if (!QSPI_HW_Write(N25Q_CMD_WRITE_VOLATILE_CFG, QSPI_WIDTH_SINGLE_BIT_SPI, &vcr, 1U))
    {
        return false;
    }

    return N25Q_WaitWhileBusy(timeout_loops);
}

bool N25Q_SetDummyCycles(uint8_t dummy_cycles, uint32_t timeout_loops)
{
    if (dummy_cycles > 15U)
    {
        return false;
    }

    uint8_t vcr = 0U;
    if (!N25Q_ReadVCR(&vcr))
    {
        return false;
    }

    vcr = (uint8_t)((vcr & (uint8_t)~N25Q_VCR_DUMMY_CYCLES_Msk) |
                    ((uint8_t)((dummy_cycles << N25Q_VCR_DUMMY_CYCLES_Pos) & N25Q_VCR_DUMMY_CYCLES_Msk)));

    return N25Q_WriteVCR(vcr, timeout_loops);
}

bool N25Q_ReadEVCR(uint8_t *evcr_out)
{
    if (evcr_out == NULL)
    {
        return false;
    }
    return QSPI_HW_Read(N25Q_CMD_READ_ENH_VOLATILE_CFG, QSPI_WIDTH_SINGLE_BIT_SPI, evcr_out, 1U);
}

bool N25Q_WriteEVCR(uint8_t evcr, uint32_t timeout_loops)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }

    if (!QSPI_HW_Write(N25Q_CMD_WRITE_ENH_VOLATILE_CFG, QSPI_WIDTH_SINGLE_BIT_SPI, &evcr, 1U))
    {
        return false;
    }

    return N25Q_WaitWhileBusy(timeout_loops);
}

bool N25Q_EnableQuadIO(uint32_t timeout_loops)
{
    /* Harmony app_n25q behavior:
     *   - WREN (single)
     *   - WRVECR (0x61) with value 0x1F (single)
     *   - after this, use QUAD_CMD ops
     */
    const uint8_t harmony_evcr = 0x1FU;

    /* Force the exact Harmony sequence in 1-1-1 first */
    s_use_quad_cmd = false;

    if (!QSPI_HW_Command(N25Q_CMD_WRITE_ENABLE, QSPI_WIDTH_SINGLE_BIT_SPI))
    {
        return false;
    }

    if (!QSPI_HW_Write(N25Q_CMD_WRITE_ENH_VOLATILE_CFG,
                       QSPI_WIDTH_SINGLE_BIT_SPI,
                       &harmony_evcr,
                       1U))
    {
        return false;
    }

    if (!N25Q_WaitWhileBusy(timeout_loops))
    {
        return false;
    }

    /* Switch subsequent command width to match Harmony */
    s_use_quad_cmd = true;

    return true;
}

bool N25Q_Enter4ByteAddressMode(uint32_t timeout_loops)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }

    if (!QSPI_HW_Command(N25Q_CMD_ENTER_4BYTE_ADDR_MODE, n25q_cmd_width()))
    {
        return false;
    }

    n25q_small_delay();
    (void)timeout_loops;
    return true;
}

bool N25Q_Exit4ByteAddressMode(uint32_t timeout_loops)
{
    if (!N25Q_WriteEnable())
    {
        return false;
    }

    if (!QSPI_HW_Command(N25Q_CMD_EXIT_4BYTE_ADDR_MODE, n25q_cmd_width()))
    {
        return false;
    }

    n25q_small_delay();
    (void)timeout_loops;
    return true;
}
