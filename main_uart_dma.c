/*
 * File:   main_uart_dma.c
 * Author: han
 *
 * Created on December 22, 2025
 *
 * This file mirrors main_test.c but uses the UART DMA API (uart_dma.h)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "common/board.h"
#include "common/delay.h"
#include "common/systick.h"
#include "drivers/uart_dma.h"
#include "common/cpu.h"

/* Use the DMA UART driver for non-blocking logging. The queue and ISR
 * handling live inside drivers/uart_dma.c. Call `UART2_DMA_Log(...)` to
 * enqueue messages and `UART2_DMA_Log_Dropped()` to query drop stats.
 */

int main(void)
{
    uint32_t t_led = 0;

    SystemConfigPerformance();
    board_init();

    /* Initialize SysTick for 1ms ticks */
    SysTick_Init_1ms_Best(BOARD_CPU_CLOCK, true);

    /* Initialize the UART peripheral (SERCOM2) first, then DMA */
    UART2_Init();
    UART2_DMA_Init();
    /* Note: driver now manages its own queue and ISR; no registration needed */

    board_led0_on();

    /* Send initial messages over DMA */
    UART2_DMA_Log("\r\n");
    UART2_DMA_Log("Hello from printf over SERCOM2 (DMA)!\r\n");
    UART2_DMA_Log("System booted at %lu Hz\r\n", BOARD_CPU_CLOCK);
    /* Return immediately from log calls; they are queued and sent by DMA */

    while (1) {
        static unsigned int count = 0;
        static unsigned int period_led_ms = 500;
        static uint32_t t_debounce = DEBOUNCE_TIME;

        if (board_sw_pressed(SW0, t_debounce)) {
            board_led0_off();
            UART2_DMA_Log("\r\nSW0 %s (>=%lu ms)\r\n",
                          SW0_Pressed() ? "PRESSED" : "RELEASED",
                          t_debounce);

            DelayMs(10);
            while (SW0_Pressed()); /* Block until sw released */

            count++;
            if (count % 2 != 0)
                period_led_ms = 100;
            else
                period_led_ms = 500;

            t_led = millis();
        }

        if (DelayMsAsync(&t_led, period_led_ms)) {
            board_led0_toggle();
            UART2_DMA_Log("LED0: %s\r\n", board_led0_is_on() ? "ON" : "OFF");
        }
    }

    return (EXIT_FAILURE);
}
