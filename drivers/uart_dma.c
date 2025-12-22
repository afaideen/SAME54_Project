#include "uart_dma.h"
#include "../common/board.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * DMA Descriptor Tables (aligned per SAME54 requirement)
 * ============================================================================ */

/* Primary descriptor table (must be 16-byte aligned) */
__attribute__((aligned(16)))
static DmacDescriptor_t dma_descriptors[12];

/* Writeback descriptor table (must be 16-byte aligned) */
__attribute__((aligned(16)))
static DmacDescriptor_t dma_writeback[12];

/* ============================================================================
 * DMA State Management
 * ============================================================================ */

static volatile bool uart2_dma_busy = false;
static UART2_DMA_Callback_t uart2_dma_callback = NULL;

/* ============================================================================
 * Logger queue configuration
 * ============================================================================ */



static char dma_log_queue[DMA_LOG_QUEUE_SIZE][DMA_LOG_BUF_SIZE];
static uint16_t dma_log_len[DMA_LOG_QUEUE_SIZE];
static volatile uint8_t dma_log_head = 0; /* next item to send */
static volatile uint8_t dma_log_tail = 0; /* next free slot to push */
static volatile uint8_t dma_log_count = 0; /* items in queue */
static volatile uint32_t dma_log_dropped = 0; /* monotonic drop counter */

/* Forward declarations for logger helpers */
static void dma_log_start_next(void);
static void uart_dma_queue_handler_local(void);

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * Enable DMAC peripheral clock and reset.
 */
static void dmac_enable(void)
{
    /* Enable DMAC AHB clock */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_DMAC_Msk;

    /* Wait for synchronization if needed */
    (void)MCLK_REGS->MCLK_AHBMASK;
}

/**
 * Configure DMAC for TX operation.
 */
static void dmac_config(void)
{
    /* Set descriptor and writeback base addresses */
    DMAC_REGS->DMAC_BASEADDR = (uint32_t)dma_descriptors;
    DMAC_REGS->DMAC_WRBADDR = (uint32_t)dma_writeback;

    /* Enable DMAC: DMAENABLE=1, LVLEN=0xF (all 4 priority levels) */
    DMAC_REGS->DMAC_CTRL =
        DMAC_CTRL_DMAENABLE_Msk |
        DMAC_CTRL_LVLEN(0xFU);
}

/**
 * Configure a specific DMA channel for UART TX.
 */
static void dmac_channel_config(uint8_t channel)
{
   
    /* Configure trigger source and action:
       - TRIGSRC: SERCOM2_DMAC_ID_TX
       - TRIGACT: BEAT (trigger on each beat/transaction)
     */
    DMAC_REGS->CHANNEL[channel].DMAC_CHCTRLA =
        DMAC_CHCTRLA_TRIGSRC(SERCOM2_DMAC_ID_TX) |
        DMAC_CHCTRLA_TRIGACT_TRANSACTION;

    /* Enable transfer complete interrupt */
    DMAC_REGS->CHANNEL[channel].DMAC_CHINTENSET = DMAC_CHINTENSET_TCMPL_Msk;
}

/**
 * Set up descriptor for a transmit operation.
 *
 * Source:  Buffer in memory (increment address)
 * Dest:    SERCOM2 DATA register (fixed address)
 * Beat:    BYTE (8-bit transfers)
 */
static void dmac_setup_tx_descriptor(
    uint8_t channel,
    const char *buffer,
    uint32_t length)
{
    DmacDescriptor_t *desc = &dma_descriptors[channel];

    /* BTCTRL: Block Transfer Control
       - VALID:      Descriptor is valid
       - SRCINC:     Auto-increment source address
       - DSTINC:     Do NOT increment destination (fixed register)
       - BEATSIZE:   8-bit transfers (BYTE)
       - BLOCKACT:   Generate interrupt on block completion
     */
    desc->btctrl =
        DMAC_BTCTRL_VALID_Msk |
        DMAC_BTCTRL_SRCINC_Msk |
        DMAC_BTCTRL_BEATSIZE_BYTE |
        DMAC_BTCTRL_BLOCKACT_INT;

    /* BTCNT: Number of beats (bytes) to transfer */
    desc->btcnt = (uint16_t)length;

    /* SRCADDR: Source address (pointer increments, so end address + 1) */
    desc->srcaddr = (uint32_t)(buffer + length);

    /* DSTADDR: Destination = SERCOM2 DATA register */
    desc->dstaddr = (uint32_t)&SERCOM2_REGS->USART_INT.SERCOM_DATA;

    /* DESCADDR: No chaining, next descriptor address is 0 */
    desc->descaddr = 0;
}

/**
 * Start a DMA transfer on the selected channel.
 */
static void dmac_channel_enable(uint8_t channel)
{
    DMAC_REGS->CHANNEL[channel].DMAC_CHCTRLA |= DMAC_CHCTRLA_ENABLE_Msk;
}
/**
 * Disable a DMA channel.
 */
static void dmac_channel_disable(uint8_t channel)
{
    DMAC_REGS->CHANNEL[channel].DMAC_CHCTRLA &= ~DMAC_CHCTRLA_ENABLE_Msk;
}
/* ============================================================================
 * Interrupt Handler (consolidated)
 * ============================================================================ */

/**
 * DMA Channel 0 Interrupt Handler.
 * Called on DMA transfer completion.
 */
void DMAC_0_Handler(void)
{
    uint8_t channel = UART2_DMA_CHANNEL;

    if ((DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG & DMAC_CHINTFLAG_TCMPL_Msk) != 0U)
    {
        /* Clear the transfer complete flag */
        DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG = DMAC_CHINTFLAG_TCMPL_Msk;

        uart2_dma_busy = false;

        if (uart2_dma_callback != NULL)
        {
            uart2_dma_callback();
        }
    }
}

/* ============================================================================
 * Public API Functions
 * ============================================================================ */

void UART2_DMA_Init(void)
{
    /* Enable DMAC peripheral */
    dmac_enable();

    /* Configure DMAC base settings */
    dmac_config();

    /* Configure the specific channel for UART2 TX */
    dmac_channel_config(UART2_DMA_CHANNEL);

    /* Enable the interrupt for this channel in NVIC */
    NVIC_EnableIRQ(DMAC_0_IRQn + UART2_DMA_CHANNEL);
}

bool UART2_DMA_Send(const char *buffer, uint32_t length)
{
    /* Prevent starting transfer if one is already in progress */
    if (uart2_dma_busy)
    {
        return false;  /* Transfer in progress, cannot start new one */
    }

    /* Mark as busy before starting */
    uart2_dma_busy = true;

    /* Set up the DMA descriptor for this transfer */
    dmac_setup_tx_descriptor(UART2_DMA_CHANNEL, buffer, length);

    /* Start the DMA transfer */
    dmac_channel_enable(UART2_DMA_CHANNEL);

    return true;  /* Transfer successfully started */
}

bool UART2_DMA_IsBusy(void)
{
    return uart2_dma_busy;
}

void UART2_DMA_SetCallback(UART2_DMA_Callback_t cb)
{
    uart2_dma_callback = cb;
}

void UART2_DMA_Wait(void)
{
    /* Spin until DMA transfer completes */
    while (uart2_dma_busy)
    {
        /* Optional: add yield/sleep for low-power operation */
        __NOP();
    }
}

/* ============================================================================
 * Logger Functions (moved from main)
 * ============================================================================ */

/**
 * Return number of dropped messages (monotonic counter).
 */
uint32_t UART2_DMA_Log_Dropped(void)
{
    return dma_log_dropped;
}

/**
 * Enqueue formatted message for DMA transmission and return immediately.
 * Returns true if the message was queued, false if it was dropped (queue full
 * or formatting error). This function is non-blocking and safe to call from
 * main context. It uses the DMAC IRQ vector as the synchronization point
 * (the DMAC IRQ handler calls the registered callback from the driver which
 * will invoke start-next logic).
 */
static bool UART2_DMA_Log_internal(const char *fmt, va_list ap)
{
    int n;
    char tmp[DMA_LOG_BUF_SIZE];

    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    if (n <= 0)
        return false;

    uint16_t len = (uint16_t)n;
    if (len >= sizeof(tmp))
        len = sizeof(tmp) - 1;

    IRQn_Type dmac_irq = (IRQn_Type)(DMAC_0_IRQn + UART2_DMA_CHANNEL);
    NVIC_DisableIRQ(dmac_irq);

    if (dma_log_count >= DMA_LOG_QUEUE_SIZE) {
        dma_log_dropped++;
        NVIC_EnableIRQ(dmac_irq);
        return false;
    }

    memcpy(dma_log_queue[dma_log_tail], tmp, len);
    dma_log_queue[dma_log_tail][len] = '\0';
    dma_log_len[dma_log_tail] = len;
    dma_log_tail = (uint8_t)((dma_log_tail + 1) % DMA_LOG_QUEUE_SIZE);
    dma_log_count++;

    if (!UART2_DMA_IsBusy()) {
        dma_log_start_next();
    }

    NVIC_EnableIRQ(dmac_irq);
    return true;
}

bool UART2_DMA_Log(const char *fmt, ...)
{
    bool res;
    va_list ap;
    va_start(ap, fmt);
    res = UART2_DMA_Log_internal(fmt, ap);
    va_end(ap);
    return res;
}

/**
 * Start transfer of the item at `dma_log_head` if one exists. This function
 * must be called with the DMAC IRQ disabled by the caller when used from
 * thread context. When called from the DMAC ISR (callback) the IRQ is
 * already active and that's fine because the ISR is single-threaded.
 */
static void dma_log_start_next(void)
{
    /* If no queued item, nothing to do */
    if (dma_log_count == 0U) {
        return;
    }

    /* Use UART2_DMA_Send to start transfer (sets busy flag and configures DMAC) */
    if (UART2_DMA_Send(dma_log_queue[dma_log_head], dma_log_len[dma_log_head])) {
        dma_log_head = (uint8_t)((dma_log_head + 1) % DMA_LOG_QUEUE_SIZE);
        dma_log_count--;
    }
}

/**
 * Local handler for UART DMA completion (for logging).
 * This is called from the DMA interrupt handler.
 */
static void uart_dma_queue_handler_local(void)
{
    /* Start the next log message transmission */
    dma_log_start_next();
}
