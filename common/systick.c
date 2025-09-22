#include "systick.h"

volatile uint32_t systick_ms = 0;

void SysTick_Handler(void) {
    systick_ms++;
}

void SysTick_Init(uint32_t cpu_hz) {
    SysTick->LOAD  = (cpu_hz / 1000) - 1;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk   |
                     SysTick_CTRL_ENABLE_Msk;
}
