#ifndef UART_DMA_H
#define UART_DMA_H

#include "sam.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/* ============================================================================
 * UART DMA Configuration
 * ============================================================================ */

#define DMA_LOG_BUF_SIZE 256
#define DMA_LOG_QUEUE_SIZE 6

/* DMA descriptor alignment requirement for SAME54 */
#define DMA_DESCRIPTOR_ALIGN  16

/* DMA channel to use for UART2 TX */
#define UART2_DMA_CHANNEL     0

/* ============================================================================
 * DMA Descriptor Structure (aligned)
 * ============================================================================ */

typedef struct __attribute__((aligned(DMA_DESCRIPTOR_ALIGN))) {
    uint16_t btctrl;    /* Block Transfer Control */
    uint16_t btcnt;     /* Block Transfer Count */
    uint32_t srcaddr;   /* Source Address */
    uint32_t dstaddr;   /* Destination Address */
    uint32_t descaddr;  /* Next Descriptor Address */
} DmacDescriptor_t;

/* ============================================================================
 * DMA Callback Function Pointer
 * ============================================================================ */

typedef void (*UART2_DMA_Callback_t)(void);

/* ============================================================================
 * Public Functions
 * ============================================================================ */

/**
 * Initialize DMA controller for UART2 TX transfers.
 * Must be called after UART2_Init().
 */
void UART2_DMA_Init(void);

/**
 * Perform DMA-based transmit of a buffer.
 *
 * @param buffer  Pointer to data to transmit
 * @param length  Number of bytes to transmit
 * @return true if transfer started successfully, false if DMA is busy
 */
bool UART2_DMA_Send(const char *buffer, uint32_t length);

/**
 * Check if DMA transfer is in progress.
 *
 * @return true if transfer is ongoing, false if idle
 */
bool UART2_DMA_IsBusy(void);

/**
 * Register a completion callback.
 * Called when DMA transfer completes.
 *
 * @param cb  Callback function pointer (NULL to disable)
 */
void UART2_DMA_SetCallback(UART2_DMA_Callback_t cb);

/**
 * Wait for DMA transfer to complete (blocking).
 */
void UART2_DMA_Wait(void);

/* ---------------------------------------------------------------------------
 * Non-blocking logger API (queueing, DMA-driven)
 * --------------------------------------------------------------------------- */

/**
 * Format and enqueue a message for DMA-based transmit.
 * Returns true if the message was queued, false if dropped (e.g. queue full)
 */
bool UART2_DMA_Log(const char *fmt, ...);

/**
 * Return a monotonic count of how many messages were dropped due to full queue.
 */
uint32_t UART2_DMA_Log_Dropped(void);

#endif /* UART_DMA_H */
