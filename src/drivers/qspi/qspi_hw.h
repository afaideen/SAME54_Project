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
    QSPI_WIDTH_DUAL_CMD       = 5,  /* 2-2-2 (Harmony uses after quad enable on some parts) */
    QSPI_WIDTH_QUAD_CMD       = 6,  /* 4-4-4 (Harmony uses after quad enable on N25Q demo) */
} qspi_width_t;

typedef enum
{
    QSPI_ADDRLEN_24BITS = 0,
    QSPI_ADDRLEN_32BITS = 3,
} qspi_addrlen_t;

void QSPI_HW_Initialize(void);
void QSPI_HW_PinInit(void);
/* Initialize QSPI peripheral (clock masks + SWRST + basic MODE_MEMORY setup). */
void QSPI_HW_Init(void);
/* Enable/disable the QSPI peripheral. */
void QSPI_HW_Enable(void);
void QSPI_HW_Disable(void);



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

/* Harmony-PLIB equivalents (already implemented in your qspi_hw.c) */
bool QSPI_HW_CommandAddr(uint8_t opcode,
                         qspi_width_t width,
                         qspi_addrlen_t addrlen,
                         uint32_t address);

bool QSPI_HW_ReadEx(uint8_t opcode,
                    qspi_width_t width,
                    uint8_t dummy_cycles,
                    void *rx,
                    size_t rx_len);

bool QSPI_HW_MemoryRead_Simple(
    uint8_t opcode,
    qspi_width_t width,
    uint8_t dummy_cycles,
    uint32_t address,
    void *rx,
    size_t rx_len
);

bool QSPI_HW_MemoryRead(
    uint8_t opcode,
    qspi_width_t width,
    qspi_addrlen_t addrlen,
    bool opt_en,
    uint8_t optcode,
    uint8_t optlen_bits,
    uint8_t dummy_cycles,
    void *rx,
    size_t rx_len,
    uint32_t address
);

bool QSPI_HW_MemoryWrite(uint8_t opcode,
                         qspi_width_t width,
                         qspi_addrlen_t addrlen,
                         bool opt_en,
                         uint8_t optcode,
                         uint8_t optlen_bits,
                         uint8_t dummy_cycles,
                         const void *tx,
                         size_t tx_len,
                         uint32_t address);

#endif /* QSPI_HW_H */
