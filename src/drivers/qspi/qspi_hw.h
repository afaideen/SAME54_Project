/* qspi_hw.h: Low-level hardware interface for SAME54 QSPI */
#ifndef QSPI_HW_H
#define QSPI_HW_H

#include <stdbool.h>
#include <stdint.h>

void QSPI_HW_Init(void);
void QSPI_HW_Enable(void);
void QSPI_HW_EnableXIP(void);
void QSPI_HW_DisableXIP(void);

/* Generic command helpers */
bool QSPI_Command(uint8_t opcode);
bool QSPI_CommandRead(uint8_t opcode, void *buf, uint32_t len);
bool QSPI_CommandWrite(uint8_t opcode, const void *buf, uint32_t len);

#endif /* QSPI_HW_H */