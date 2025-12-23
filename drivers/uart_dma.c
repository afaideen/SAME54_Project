#include "uart.h"
#include "uart_dma.h"
#include "../common/board.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Timebase (must be provided by your project; typically SysTick 1ms) */
extern uint32_t millis(void);

/* ============================================================================
 * DMA Descriptor Tables (aligned per SAME54 requirement)
 * ============================================================================ */

/* Primary and writeback descriptor tables (must be 16-byte aligned).
 *
 * SAME54's DMAC implements up to 32 channels.  The hardware expects the
 * BASEADDR and WRBADDR pointers to reference tables that have an entry for
 * every possible channel, ordered by channel number.  If the tables are
 * smaller than the hardware channel count, the DMAC may index beyond the
 * allocation and corrupt memory.  We therefore size the tables based on
 * the DMAC_CHANNEL_NUMBER macro when available, or fall back to 32 which
 * is the channel count on SAME54 devices.  Each descriptor is aligned
 * according to DMA_DESCRIPTOR_ALIGN defined in uart_dma.h.
 */
#ifdef DMAC_CHANNEL_NUMBER
#define DMA_CHANNEL_COUNT DMAC_CHANNEL_NUMBER
#else
#define DMA_CHANNEL_COUNT 32u
#endif

__attribute__((aligned(DMA_DESCRIPTOR_ALIGN)))
static DmacDescriptor_t dma_descriptors[DMA_CHANNEL_COUNT];

__attribute__((aligned(DMA_DESCRIPTOR_ALIGN)))
static DmacDescriptor_t dma_writeback[DMA_CHANNEL_COUNT];

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

/* Timestamp state for logging */
static uint32_t s_uart2_log_prev_ms = 0;
static bool s_uart2_log_prev_valid = false;


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
    /* Harmony v3 enables the DMAC clock via MCLK_AHBMASK (DMAC is on AHB). */
    MCLK_REGS->MCLK_AHBMASK |= MCLK_AHBMASK_DMAC_Msk;

    /* Read back to ensure the write has completed */
    (void)MCLK_REGS->MCLK_AHBMASK;
}

/**
 * Configure DMAC for TX operation.
 */
static void dmac_config(void)
{
    /* (Harmony v3) Update the Base address and Write Back address register */
    DMAC_REGS->DMAC_BASEADDR = (uint32_t)dma_descriptors;
    DMAC_REGS->DMAC_WRBADDR  = (uint32_t)dma_writeback;

    /* (Harmony v3) Update the Priority Control register:
       - enable round-robin for all levels
       - set each level priority number to 1 (doesn't matter for a single channel,
         but keeps behavior identical to Harmony) */
    DMAC_REGS->DMAC_PRICTRL0 |=
        DMAC_PRICTRL0_LVLPRI0(1U) | DMAC_PRICTRL0_RRLVLEN0_Msk |
        DMAC_PRICTRL0_LVLPRI1(1U) | DMAC_PRICTRL0_RRLVLEN1_Msk |
        DMAC_PRICTRL0_LVLPRI2(1U) | DMAC_PRICTRL0_RRLVLEN2_Msk |
        DMAC_PRICTRL0_LVLPRI3(1U) | DMAC_PRICTRL0_RRLVLEN3_Msk;

    /* (Harmony v3) Enable DMAC + all priority levels */
    DMAC_REGS->DMAC_CTRL = DMAC_CTRL_DMAENABLE_Msk |
                           DMAC_CTRL_LVLEN0_Msk |
                           DMAC_CTRL_LVLEN1_Msk |
                           DMAC_CTRL_LVLEN2_Msk |
                           DMAC_CTRL_LVLEN3_Msk;
}

/**
 * Configure a specific DMA channel for UART TX.
 */
static void dmac_channel_config(uint8_t channel)
{
    /* (Harmony v3) Configure DMA channel for SERCOM2 USART TX:
       - TRIGSRC = SERCOM2 TX
       - TRIGACT = BURST (one trigger per burst)
       - BURSTLEN = 0 => burst length = 1 beat
       - THRESHOLD = 0
       NOTE: Using TRIGACT_TRANSACTION here will typically break peripheral-paced
       transfers. Harmony uses TRIGACT = BURST. */
    DMAC_REGS->CHANNEL[channel].DMAC_CHCTRLA =
        DMAC_CHCTRLA_TRIGACT_BURST |
        DMAC_CHCTRLA_TRIGSRC(SERCOM2_DMAC_ID_TX) |
        DMAC_CHCTRLA_THRESHOLD(0U) |
        DMAC_CHCTRLA_BURSTLEN(0U);

    /* (Harmony v3) Channel priority level = 0 (matches DMAC_0 interrupt) */
    DMAC_REGS->CHANNEL[channel].DMAC_CHPRILVL = DMAC_CHPRILVL_PRILVL(0U);

    /* Enable transfer complete + error interrupts for this channel */
    DMAC_REGS->CHANNEL[channel].DMAC_CHINTENSET =
        (DMAC_CHINTENSET_TCMPL_Msk | DMAC_CHINTENSET_TERR_Msk);

    /* Clear any stale flags */
    DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG =
        (DMAC_CHINTFLAG_TCMPL_Msk | DMAC_CHINTFLAG_TERR_Msk);
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
    /* Enable the requested channel via its CHANNEL[] register. */
    DMAC_REGS->CHANNEL[channel].DMAC_CHCTRLA |= DMAC_CHCTRLA_ENABLE_Msk;
}

/**
 * Disable a DMA channel.
 */
static void dmac_channel_disable(uint8_t channel)
{
    /* Disable the requested channel via its CHANNEL[] register. */
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
    uint8_t flags = DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG;

    /* Transfer complete */
    if ((flags & DMAC_CHINTFLAG_TCMPL_Msk) != 0U)
    {
        DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG = DMAC_CHINTFLAG_TCMPL_Msk;
        uart2_dma_busy = false;

        /* Start next queued log message (if any) */
        dma_log_start_next();

        if (uart2_dma_callback != NULL)
        {
            uart2_dma_callback();
        }
    }

    /* Transfer error */
    if ((flags & DMAC_CHINTFLAG_TERR_Msk) != 0U)
    {
        DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG = DMAC_CHINTFLAG_TERR_Msk;
        uart2_dma_busy = false;

        /* Drop current item already in-flight; try to continue queue */
        dma_log_start_next();

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
    /* Initialize timestamp baseline for log prefixes */
    s_uart2_log_prev_ms = millis();
    s_uart2_log_prev_valid = false;

    /* Enable DMAC peripheral clock */
    dmac_enable();

    /* Configure DMAC base settings (BASEADDR/WRBADDR/PRICTRL0/CTRL) */
    dmac_config();

    /* Configure the specific channel for UART2 TX */
    dmac_channel_config(UART2_DMA_CHANNEL);

    /* Enable the interrupt for DMAC priority level 0 (matches CHPRILVL=0) */
    NVIC_ClearPendingIRQ(DMAC_0_IRQn);
    NVIC_EnableIRQ(DMAC_0_IRQn);
}

bool UART2_DMA_Send(const char *buffer, uint32_t length)
{
    uint8_t channel = UART2_DMA_CHANNEL;

    if ((buffer == NULL) || (length == 0U))
    {
        return false;
    }

    /* SAME54 DMAC BTCNT is 16-bit. Keep each transfer within 65535 bytes. */
    if (length > 0xFFFFU)
    {
        length = 0xFFFFU;
    }

    /* Prevent starting transfer if one is already in progress */
    if (uart2_dma_busy)
    {
        return false;
    }

    /* Clear any stale flags before starting */
    DMAC_REGS->CHANNEL[channel].DMAC_CHINTFLAG =
        (DMAC_CHINTFLAG_TCMPL_Msk | DMAC_CHINTFLAG_TERR_Msk);

    /* Mark as busy before starting */
    uart2_dma_busy = true;

    /* Set up the DMA descriptor for this transfer */
    dmac_setup_tx_descriptor(channel, buffer, length);

    /* Start the DMA transfer */
    dmac_channel_enable(channel);

    return true;
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
    /* Build the user message first (consume va_list exactly once) */
    char body[DMA_LOG_BUF_SIZE];
    int bn = vsnprintf(body, sizeof(body), fmt, ap);
    if (bn <= 0) {
        return false;
    }

    /* Timestamp prefix */
    uint32_t now = millis();
    uint32_t delta = s_uart2_log_prev_valid ? (now - s_uart2_log_prev_ms) : 0U;
    s_uart2_log_prev_ms = now;
    s_uart2_log_prev_valid = true;

    /* Compose final message: [TIME_MS][DELTA_MS] + body */
    char tmp[DMA_LOG_BUF_SIZE];
    int n = snprintf(tmp, sizeof(tmp), "[%lu][%lu]%s",
                     (unsigned long)now, (unsigned long)delta, body);
    if (n <= 0) {
        return false;
    }

    uint16_t len = (uint16_t)n;
    if (len >= sizeof(tmp)) {
        len = sizeof(tmp) - 1U;
    }

    /* If DMA is busy, queue message; else start immediately */
    bool res;

    /* Protect queue state from DMAC ISR races */
    IRQn_Type dmac_irq = DMAC_0_IRQn; /* CHPRILVL=0 */
    NVIC_DisableIRQ(dmac_irq);

    if (dma_log_count >= DMA_LOG_QUEUE_SIZE) {
        dma_log_dropped++;
        NVIC_EnableIRQ(dmac_irq);
        return false;
    }

    memcpy(dma_log_queue[dma_log_tail], tmp, len);
    dma_log_queue[dma_log_tail][len] = '\0';
    dma_log_len[dma_log_tail] = len;
    dma_log_tail = (uint8_t)((dma_log_tail + 1U) % DMA_LOG_QUEUE_SIZE);
    dma_log_count++;

    if (!UART2_DMA_IsBusy()) {
        dma_log_start_next();
    }

    NVIC_EnableIRQ(dmac_irq);

    res = true;
    return res;
}

bool UART2_DMA_Log(const char *fmt, ...)
{
    bool res;
    va_list ap;
    va_start(ap, fmt);

    va_list ap_copy;
    va_copy(ap_copy, ap);
    res = UART2_DMA_Log_internal(fmt, ap_copy);
    va_end(ap_copy);

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
