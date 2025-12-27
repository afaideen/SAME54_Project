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
#include "drivers/rtcc.h"
#include "drivers/qspi/sst26/sst26.h"
#include "drivers/qspi/qspi_flash.h"
#include "common/cpu.h"


int main(void)
{ 
    /* Use the DMA UART driver for non-blocking logging. The queue and ISR
 * handling live inside drivers/uart_dma.c. Call `UART2_DMA_Log(...)` to
 * enqueue messages and `UART2_DMA_Log_Dropped()` to query drop stats.
 */
    char banner[][100] = {
            "\r\n",
            "Hello from printf over SERCOM2 (DMA)!\r\n",
            "System booted at %lu Hz\r\n",
        };

    SystemConfigPerformance();
    board_init();
    
    /* Note: driver now manages its own queue and ISR; no registration needed */
    board_led0_on();

    /* Send initial messages over DMA */
//    UART2_DMA_Log(banner[0]);
    printf(banner[0]);

//    SST26_Example_Erase();
    QSPI_FLASH_Example_WriteRead();


    while (1) {
        static delay_t t_led = {0,500,0};
        static delay_t t_rtc = {0,1000,0};
        static sw_t sw0 = {SW0, DEBOUNCE_TIME, 0};
        
        if (board_sw_pressed(&sw0)) {
            board_led0_off();
            if (sw0.cnt % 2 != 0)
                t_led.period = 100;
            else
                t_led.period = 500;

            t_led.t_delay = millis();
        }

        if (DelayMsAsync(&t_led)) {
            t_led.cnt++;
            board_led0_toggle();
            UART2_DMA_Log("LED0: %s\r\n", board_led0_is_on() ? "ON" : "OFF");
        }
  
    }

    return (EXIT_FAILURE);
}

