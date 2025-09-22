#ifndef SYSTICK_H
#define SYSTICK_H

#include "sam.h"
#include <stdint.h>

extern volatile uint32_t systick_ms;

void SysTick_Init(uint32_t cpu_hz);

#endif
