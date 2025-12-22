
#include "board.h"
#include "systick.h"
#include "delay.h"

extern volatile uint32_t systick_ms;

/**
 * Blocking delay in milliseconds
 * @param delay_ms Number of milliseconds to delay
 * WARNING: This is a blocking busy-wait function
 */
void DelayMs(uint32_t delay_ms)
{
    uint32_t start = systick_ms;

    while ((uint32_t)(systick_ms - start) < delay_ms)
    {
        /* busy wait */
    }
}

/**
 * Non-blocking asynchronous delay in milliseconds
 * @param t1 Pointer to time variable to track elapsed time
 * @param ms Delay duration in milliseconds
 * @return true when delay has elapsed, false otherwise
 * Usage: Initialize *t1 to 0, call repeatedly until it returns true
 */
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

/**
 * Blocking delay in CPU clock cycles
 * @param cycles Number of CPU cycles to delay
 * WARNING: Blocking, do NOT call from ISR
 */
void DelayTcy(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
        /* busy wait */
    }
}

/**
 * Blocking delay in microseconds
 * @param us Number of microseconds to delay
 * WARNING: Blocking, do NOT call from ISR
 */
void DelayUs(uint32_t us)
{
    uint32_t cycles_per_us = CPU_CLOCK_HZ / 1000000UL;
    DelayTcy(us * cycles_per_us);
}
