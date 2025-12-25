/* qspi_flash.h: High-level QSPI flash interface */
#ifndef QSPI_FLASH_H
#define QSPI_FLASH_H

#include <stdbool.h>

bool QSPI_Flash_Init(void);
void QSPI_Flash_Diag_Print(void);

#define APP_USE_SST26_FLASH  0
#define APP_USE_N25Q_FLASH   1


#endif /* QSPI_FLASH_H */