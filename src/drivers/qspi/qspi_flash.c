#include "qspi_flash.h"

#include "qspi_hw.h"
#include "n25q256a.h"

/*
 * QSPI flash init sequence (SAME54 Xplained Pro + N25Q256A):
 *  1) Init QSPI peripheral (AHB clocks + reset + basic CTRLB/BAUD)
 *  2) Reset flash (66h/99h)
 *  3) Read JEDEC ID (9Fh)
 *  4) Enable Quad I/O via Enhanced Volatile Config Register (61h)
 *  5) Program Volatile Config Register dummy cycles (81h) for safe fast-read
 *  6) Enter 4-byte addressing (B7h) so full 32MB array is reachable
 *  7) Configure SAME54 QSPI memory-read instruction frame for XIP-style reads
 */
bool QSPI_Flash_Init(void)
{
    QSPI_HW_Init();
    QSPI_HW_Enable();

    if (!N25Q_Reset())
    {
        return false;
    }

    uint32_t jedec = 0U;
    if (!N25Q_ReadJEDEC(&jedec))
    {
        return false;
    }

    /* Micron manufacturer ID is 0x20 (JEDEC byte0). */
    if ((jedec & 0x000000FFUL) != 0x20UL)
    {
        return false;
    }

    /* Enable quad I/O immediately (volatile) */
    if (!N25Q_EnableQuadIO(2000000UL))
    {
        return false;
    }

    /*
     * Dummy cycles:
     * Datasheet supports higher frequencies with higher dummy counts; 10 is a
     * conservative choice for QUAD I/O FAST READ.
     */
    if (!N25Q_SetDummyCycles(10U, 2000000UL))
    {
        return false;
    }

    /* Use 4-byte addressing so full density is addressable. */
    if (!N25Q_Enter4ByteAddressMode(2000000UL))
    {
        return false;
    }

    /*
     * Configure SAME54 QSPI for memory mapped fast reads.
     *
     * N25Q256A quad I/O fast read opcodes:
     *  - 3-byte: 0xEB
     *  - 4-byte: 0xEC
     *
     * Uses an 8-bit "mode" option code. We keep it 0x00 (no XIP latch).
     */
    QSPI_HW_ConfigureMemoryRead(
        /* opcode */ 0xECU,
        /* width  */ QSPI_WIDTH_QUAD_IO,
        /* addr   */ QSPI_ADDRLEN_32BITS,
        /* opt    */ 0x00U,
        /* optlen */ 8U,
        /* dummy  */ 10U);

    return true;
}
