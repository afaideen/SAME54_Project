/* board.h: Defines board-level constants for QSPI */
#ifndef BOARD_H
#define BOARD_H

/*
 * Base address of the QSPI AHB memory region.
 * Use the device-pack definition (from sam.h) so we stay header-model-correct.
 */
#include "sam.h"

/* Alias used by the driver */
#define QSPI_AHB_BASE   (QSPI_ADDR)

#endif /* BOARD_H */