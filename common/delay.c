
#include "board.h"
#include "systick.h"
#include "delay.h"

extern volatile uint32_t systick_ms;

void DelayMs(uint32_t delay_ms)
{
    uint32_t start = systick_ms;

    while ((uint32_t)(systick_ms - start) < delay_ms)
    {
        /* busy wait */
    }
}
bool DelayMsAsync(uint32_t *t1, unsigned int ms) {
    if (*t1 == 0) {
        *t1 = systick_ms;
    }
    if ((systick_ms - *t1) >= ms) {
        *t1 = millis();
        return true;
    }
    return false;
}

/* WARNING: Blocking, do NOT call from ISR */
void DelayTcy(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
        /* busy wait */
    }
}
void DelayUs(uint32_t us)
{
    uint32_t cycles_per_us = CPU_CLOCK_HZ / 1000000UL;
    DelayTcy(us * cycles_per_us);
}
