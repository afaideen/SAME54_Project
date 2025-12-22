#ifndef SYSTICK_H
#define SYSTICK_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>

extern volatile uint32_t systick_ms;

uint32_t millis(void);
void SysTick_Init(uint32_t cpu_hz);
void SysTick_Init_1ms_Best(uint32_t cpu_hz, bool reset_tick);

#endif
