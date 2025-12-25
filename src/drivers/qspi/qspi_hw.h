/* qspi_hw.h: Low-level SAME54 QSPI peripheral helpers (XC32 SAME54 pack model)
 *
 * This layer only deals with the MCU QSPI peripheral (not the flash command set).
 * It intentionally mirrors the Microchip Harmony QSPI PLIB flow (INSTRCTRL/
 * INSTRFRAME + QSPI_ADDR AHB window), but uses only the XC32 *_REGS model.
 */

#ifndef QSPI_HW_H
#define QSPI_HW_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "sam.h"
#include "instance/qspi.h"

/* Quick self-check: header-model mismatch guard */
#if !defined(QSPI_REGS) || !defined(MCLK_REGS)
#error "QSPI_REGS/MCLK_REGS not found. Check SAME54 device-pack headers (XC32 pic32c)."
#endif

typedef enum
{
    QSPI_WIDTH_SINGLE_BIT_SPI = 0,  /* 1-1-1 */
    QSPI_WIDTH_DUAL_OUTPUT    = 1,  /* 1-1-2 */
    QSPI_WIDTH_QUAD_OUTPUT    = 2,  /* 1-1-4 */
    QSPI_WIDTH_DUAL_IO        = 3,  /* 1-2-2 */
    QSPI_WIDTH_QUAD_IO        = 4,  /* 1-4-4 */
} qspi_width_t;

typedef enum
{
    QSPI_ADDRLEN_24BITS = 0,
    QSPI_ADDRLEN_32BITS = 3,
} qspi_addrlen_t;

/* Initialize QSPI peripheral (clock masks + SWRST + basic MODE_MEMORY setup). */
void QSPI_HW_Init(void);

/* Enable/disable the QSPI peripheral. */
void QSPI_HW_Enable(void);
void QSPI_HW_Disable(void);

/*
 * Configure the default memory-mapped READ transaction.
 * After this, the CPU can read from (uint8_t*)QSPI_ADDR + offset.
 */
void QSPI_HW_ConfigureMemoryRead(uint8_t opcode,
                                 qspi_width_t width,
                                 qspi_addrlen_t addrlen,
                                 uint8_t optcode,
                                 uint8_t optlen_bits,
                                 uint8_t dummy_cycles);

/*
 * Execute an instruction-only command (no address, no data).
 * Uses the AHB window method and waits for INSTREND.
 */
bool QSPI_HW_Command(uint8_t opcode, qspi_width_t width);

/*
 * Execute a command and read/write its data payload.
 * For register-like operations (status, config regs, etc).
 */
bool QSPI_HW_Read(uint8_t opcode, qspi_width_t width, void *rx, size_t rx_len);
bool QSPI_HW_Write(uint8_t opcode, qspi_width_t width, const void *tx, size_t tx_len);

#endif /* QSPI_HW_H */
