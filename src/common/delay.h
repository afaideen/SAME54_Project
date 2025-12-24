#ifndef DELAY_H
#define DELAY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t t_delay;
    uint32_t period;
    int      cnt;
    
}delay_t;

/**
 * Blocking delay in milliseconds
 * @param delay_ms Number of milliseconds to delay
 * WARNING: This is a blocking busy-wait function
 */
void DelayMs(uint32_t delay_ms);

/**
 * Non-blocking asynchronous delay in milliseconds
 * @param t1 Pointer to time variable to track elapsed time
 * @param ms Delay duration in milliseconds
 * @return true when delay has elapsed, false otherwise
 * Usage: Initialize *t1 to 0, call repeatedly until it returns true
 */
bool DelayMsAsync(delay_t *t1);

/**
 * Blocking delay in CPU clock cycles
 * @param cycles Number of CPU cycles to delay
 * WARNING: Blocking, do NOT call from ISR
 */
void DelayTcy(uint32_t cycles);

/**
 * Blocking delay in microseconds
 * @param us Number of microseconds to delay
 * WARNING: Blocking, do NOT call from ISR
 */
void DelayUs(uint32_t us);

#endif
