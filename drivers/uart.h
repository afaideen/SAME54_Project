#ifndef UART_H
#define UART_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>

// Blocking API
void UART2_Init(void);
void uart_init(void);
void UART2_Putc(char c);
void UART2_Puts(const char *s);
void UART2_Log(const char *fmt, ...);
// DMA API
void UART2_DMA_Init(void);
bool UART2_DMA_Send(const char *buffer, uint32_t length);
bool UART2_DMA_IsBusy(void);

// DMA Callback
typedef void (*UART2_DMA_Callback_t)(void);
void UART2_DMA_SetCallback(UART2_DMA_Callback_t cb);

#endif
