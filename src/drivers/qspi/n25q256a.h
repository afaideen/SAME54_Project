/* n25q256a.h: Device-specific interface for Micron N25Q256A QSPI flash */
#ifndef N25Q256A_H
#define N25Q256A_H

#include <stdbool.h>
#include <stdint.h>

bool N25Q_Reset(void);
bool N25Q_ReadJEDEC(uint32_t *id);
bool N25Q_WriteEnable(void);
bool N25Q_ReadStatus(uint8_t *sr);
bool N25Q_EnableQuad(void);
bool N25Q_SetDummyCycles(uint8_t cycles);
bool N25Q_WaitReady(uint32_t timeout_ms);

#endif /* N25Q256A_H */