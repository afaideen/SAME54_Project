#include "systick.h"
#include <stdbool.h>

volatile uint32_t systick_ms = 0;

void SysTick_Handler(void) {
    systick_ms++;
}
uint32_t millis(void)
{
    return systick_ms;   // 32-bit read is atomic on Cortex-M
}

void SysTick_Init(uint32_t cpu_hz) {
    SysTick->LOAD  = (cpu_hz / 1000) - 1;
    SysTick->VAL   = 0;
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk   |
                     SysTick_CTRL_ENABLE_Msk;
}

/**
 * Initialize SysTick for 1ms tick.
 *
 * @param cpu_hz      CPU clock frequency in Hz (e.g. 120000000UL)
 * @param reset_tick  if true, reset g_ms_ticks to 0; if false, keep running count
 */
void SysTick_Init_1ms_Best(uint32_t cpu_hz, bool reset_tick)
{
	uint32_t ticks = cpu_hz / 1000U;       // ticks per 1ms

	// SysTick LOAD is 24-bit, and SysTick_Config expects "ticks" (1..0x01000000)
	if ((ticks == 0U) || (ticks > 0x01000000UL))
	{
		// bad parameter / impossible config
		while (1) { }
	}

	if (reset_tick)
	{
		systick_ms = 0;
	}

	// Choose a priority that won't starve other critical ISRs (tune if needed)
	NVIC_SetPriority(SysTick_IRQn, 3U);

	// Programs LOAD = ticks-1, VAL = 0, CTRL = CLKSOURCE|TICKINT|ENABLE
	if (SysTick_Config(ticks) != 0U)
	{
		// SysTick_Config failed (shouldn't happen if we validated ticks)
		while (1) { }
	}
}