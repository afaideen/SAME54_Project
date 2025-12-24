//
// Created by han on 22-Dec-25.
//

#ifndef CPU_H
#define CPU_H

/**
 * Configures CPU for optimal performance
 * 
 * Initializes:
 * - Flash wait states (RWS=5)
 * - Instruction cache (CMCC)
 * - DPLL0 clock generator (120 MHz)
 * - Generic clock generators (GCLK0, GCLK1, GCLK2)
 * - CPU and peripheral clock dividers
 * - Peripheral clock channels (EIC, SERCOM2, SERCOM3)
 * - MCLK bus masks
 * 
 * Should be called early in startup before using any peripherals
 */
void SystemConfigPerformance(void);
void CPU_LogClockOverview(void);

#endif //CPU_H
