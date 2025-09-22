#include <stdio.h>
#include "common/board.h"
#include "common/systick.h"
#include "common/delay.h"
#include "drivers/uart.h"
#include "drivers/swo.h"

const char dmaMessage[] = "Hello from DMA-driven UART!\r\n";

void dma_done_callback(void) {
	printf("DMA transfer completed!\r\n");
}

int main(void) {
	SystemConfigPerformance();
	SysTick_Init(BOARD_CPU_CLOCK);
	UART2_Init();
	UART2_DMA_Init();
	UART2_DMA_SetCallback(dma_done_callback);
	SWO_Init(BOARD_CPU_CLOCK, BOARD_SWO_BAUDRATE);
	BOARD_Init();

	uint32_t t1 = 0;
	setvbuf(stdout, NULL, _IONBF, 0);

	while (1) {
		if (DelayMs(&t1, BOARD_LED_BLINK_MS)) {
			// --- LED control in PIC32MZ style ---
			PORTCbits.OUTTGL = LED0_MASK;   // Toggle LED (PC18, active low)

			// --- Button read in PIC32MZ style ---
			if (!(PORTBINbits.reg & BUTTON0_MASK)) {  // PB31 active low
				printf("Button pressed!\r\n");
				SWO_PrintString("Button pressed!\r\n");
			} else {
				printf("Button not pressed.\r\n");
				SWO_PrintString("Button not pressed!\r\n");
			}

			if (!UART2_DMA_Send(dmaMessage, sizeof(dmaMessage) - 1)) {
				printf("DMA busy, skipping this round.\r\n");
			}
		}
	}
}
