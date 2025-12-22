/* 
 * File:   main_test.c
 * Author: han
 *
 * Created on December 22, 2025, 12:43 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common/board.h"
#include "common/delay.h"
#include "common/systick.h"
#include "drivers/uart.h"
#include "common/cpu.h"

/*
 * 
 */
int main(void) 
{
    uint32_t t_led = 0;
    SystemConfigPerformance();
    board_init();
    SysTick_Init_1ms_Best(BOARD_CPU_CLOCK, true);
    UART2_Init();
    board_led0_on();
    UART2_Puts("\r\n");
    printf("Hello from printf over SERCOM2!\r\n");
    UART2_Log("System booted at %lu Hz\r\n", BOARD_CPU_CLOCK);
    while(1)
    {
        static unsigned int count = 0;
        static unsigned int period_led_ms = 500;
        static uint32_t t_debounce = DEBOUNCE_TIME;
        if (board_sw_pressed(SW0, t_debounce)) {
            board_led0_off();
            UART2_Log("\r\nSW0 %s (>=%lu ms)\r\n", 
                    SW0_Pressed() ?"PRESSED":"RELEASED", 
                    t_debounce);
            DelayMs(10);
            while(SW0_Pressed());   // Block until sw released
            count++;
            if(count % 2 != 0)
                period_led_ms = 100;
            else
                period_led_ms = 500;
            t_led = millis();
        }
        if(DelayMsAsync(&t_led, period_led_ms))
        {
            board_led0_toggle();
            UART2_Log("LED0: %s", board_led0_is_on()?"ON":"OFF");
        }
    }

    return (EXIT_FAILURE);
}

