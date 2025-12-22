#include <stdio.h>
#include <stdarg.h>
#include "uart.h"
#include "../common/board.h"
#include "../common/systick.h"

//__attribute__((aligned(16))) DmacDescriptor dmaDescriptor[12];
//__attribute__((aligned(16))) DmacDescriptor dmaWriteback[12];

//static volatile bool uart2_dma_busy = false;
//static UART2_DMA_Callback_t uart2_dma_callback = NULL;

void UART2_Log(const char *fmt, ...)
{
    char buf[128];
    char msg[96];
    static uint32_t last_time = 0;
    

    uint32_t now   = millis();
    uint32_t delta = now - last_time;
    last_time = now;

    va_list ap;
    va_start(ap, fmt);
//    (void)vsnprintf(buf, sizeof(buf), fmt, ap);
     (void)vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    
    /* prepend time + delta */
    (void)snprintf(buf, sizeof(buf),
                    "[%lu][+%lu] %s\r\n",
                    (unsigned long)now,
                    (unsigned long)delta,
                    msg);

    UART2_Puts(buf);

    /* If you only have uart_putc(), use this instead:
    for (char *p = buf; *p; p++) uart_putc(*p);
    */
}

static inline void port_set_pmux(uint8_t port, uint8_t pin, uint8_t func)
{
    uint8_t idx = pin >> 1;
    if ((pin & 1u) == 0u) {
        PORT_REGS->GROUP[port].PORT_PMUX[idx] =
            (PORT_REGS->GROUP[port].PORT_PMUX[idx] & PORT_PMUX_PMUXO_Msk) |
            PORT_PMUX_PMUXE(func);
    } else {
        PORT_REGS->GROUP[port].PORT_PMUX[idx] =
            (PORT_REGS->GROUP[port].PORT_PMUX[idx] & PORT_PMUX_PMUXE_Msk) |
            PORT_PMUX_PMUXO(func);
    }
    PORT_REGS->GROUP[port].PORT_PINCFG[pin] |= PORT_PINCFG_PMUXEN_Msk;
}
static void uart_pins_init(void)
{
    port_set_pmux(UART_TX_PORT_GROUP, UART_TX_PIN, UART_PMUX_FUNC_D);
    port_set_pmux(UART_RX_PORT_GROUP, UART_RX_PIN, UART_PMUX_FUNC_D);
}

static inline uint16_t sercom_usart_baud_calc(uint32_t ref_hz, uint32_t baud)
{
    /* SAME54 SERCOM USART:
     * - Internal clock
     * - 16x oversampling (SAMPR = 0)
     * - Arithmetic mode
     */
    uint64_t tmp = (uint64_t)16 * baud * 65536ULL;
    return (uint16_t)(65536ULL - (tmp / ref_hz));
}

static inline void PORT_SetMux(uint8_t port_group, uint8_t pin, uint8_t pmux_func)
{
    uint8_t pmux_index = (uint8_t)(pin >> 1);
    uint8_t pmux       = PORT_REGS->GROUP[port_group].PORT_PMUX[pmux_index];

    if ((pin & 1U) == 0U)
    {
        /* Even pin -> PMUXE (low nibble) */
        pmux = (uint8_t)((pmux & (uint8_t)~PORT_PMUX_PMUXE_Msk) | (uint8_t)PORT_PMUX_PMUXE(pmux_func));
    }
    else
    {
        /* Odd pin -> PMUXO (high nibble) */
        pmux = (uint8_t)((pmux & (uint8_t)~PORT_PMUX_PMUXO_Msk) | (uint8_t)PORT_PMUX_PMUXO(pmux_func));
    }

    PORT_REGS->GROUP[port_group].PORT_PMUX[pmux_index] = pmux;
    PORT_REGS->GROUP[port_group].PORT_PINCFG[pin]      = PORT_PINCFG_PMUXEN_Msk;
}

void UART2_Init(void) {
    uart_pins_init();
    /* 1) Enable SERCOM2 APB clock (SAME54: SERCOM2 is on APBB) */
    MCLK_REGS->MCLK_APBBMASK |= MCLK_APBBMASK_SERCOM2_Msk;
    
    /* 2) Route GCLK1 to SERCOM2 core channel (matches your CMSIS code) */
    GCLK_REGS->GCLK_PCHCTRL[SERCOM2_GCLK_ID_CORE] =
        GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[SERCOM2_GCLK_ID_CORE] & GCLK_PCHCTRL_CHEN_Msk) == 0U) { }

    /* 3) Pin mux */
    PORT_SetMux(UART_RX_PORT_GROUP, UART_RX_PIN, UART_PMUX_FUNC_D);
    PORT_SetMux(UART_TX_PORT_GROUP, UART_TX_PIN, UART_PMUX_FUNC_D);   
    

    /* 4) Software reset */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_SWRST_Msk;
    while ((SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_SWRST_Msk) != 0U) { }

    /* 5) CTRLA (same intent as your CMSIS version) */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA =
        SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
        SERCOM_USART_INT_CTRLA_RXPO(1UL) |   /* PAD1 = RX (PB24) */
        SERCOM_USART_INT_CTRLA_TXPO(0UL) |   /* PAD0 = TX (PB25) */
        SERCOM_USART_INT_CTRLA_DORD_Msk;     /* LSB first */

    /* 6) CTRLB (TX + RX) */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLB =
        SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT |
        SERCOM_USART_INT_CTRLB_SBMODE_1_BIT |
        SERCOM_USART_INT_CTRLB_TXEN_Msk |
        SERCOM_USART_INT_CTRLB_RXEN_Msk;

    while ((SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB_Msk) != 0U) { }

    /* 7) BAUD (same math as your CMSIS function; assumes 16x oversampling, arithmetic mode) */
    uint64_t br = sercom_usart_baud_calc(GCLK1_CLOCK_HZ, UART_BAUDRATE); // ~64530 at 120MHz
    SERCOM2_REGS->USART_INT.SERCOM_BAUD = (uint16_t)SERCOM_USART_INT_BAUD_BAUD((uint16_t)br);

    /* 8) Enable */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    while ((SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_ENABLE_Msk) != 0U) { }
}
void uart_init(void)
{
    uart_pins_init();
    /* Optional safety: disable before config */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA &= ~SERCOM_USART_INT_CTRLA_ENABLE_Msk;
    while (SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY != 0U) { }

    /* CTRLA: matches PLIB exactly */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA =
        SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK |
        SERCOM_USART_INT_CTRLA_RXPO(0x1UL) |
        SERCOM_USART_INT_CTRLA_TXPO(0x0UL) |
        SERCOM_USART_INT_CTRLA_DORD_Msk |
        SERCOM_USART_INT_CTRLA_IBON_Msk |
        SERCOM_USART_INT_CTRLA_FORM(0x0UL) |
        SERCOM_USART_INT_CTRLA_SAMPR(0UL);

    /* BAUD: matches PLIB exactly */
    uint16_t baudReg = sercom_usart_baud_calc(GCLK1_CLOCK_HZ, UART_BAUDRATE); // ~64530 at 120MHz
    SERCOM2_REGS->USART_INT.SERCOM_BAUD = (uint16_t)SERCOM_USART_INT_BAUD_BAUD(baudReg);
//        (uint16_t)SERCOM_USART_INT_BAUD_BAUD(SERCOM2_USART_INT_BAUD_VALUE);

    /* CTRLB: matches PLIB exactly (TX only) */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLB =
        SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT |
        SERCOM_USART_INT_CTRLB_SBMODE_1_BIT |
        SERCOM_USART_INT_CTRLB_TXEN_Msk;

    while (SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY != 0U) { }

    /* Enable: matches PLIB */
    SERCOM2_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE_Msk;

    while (SERCOM2_REGS->USART_INT.SERCOM_SYNCBUSY != 0U) { }
    
    while (SERCOM2_REGS->USART_INT.SERCOM_INTFLAG &
       SERCOM_USART_INT_INTFLAG_RXC_Msk)
    {
        (void)SERCOM2_REGS->USART_INT.SERCOM_DATA;
    }
}
// Blocking transmit
void UART2_Putc(char c) {
    while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0u) {}
    SERCOM2_REGS->USART_INT.SERCOM_DATA = SERCOM_USART_INT_DATA_DATA((uint16_t)c);
}
void UART2_Puts(const char *s) {
    while (*s) UART2_Putc(*s++);
}
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        UART2_Putc(ptr[i]);
    }
    return len;
}
/* Microchip pic32c-lib stubs call this when you use printf() */
void _mon_putc(char c)
{
    if (c == '\n')
    {
        UART2_Putc('\r');   /* CRLF for terminal */
    }
    UART2_Putc(c);
}
// ---------------- DMA EXTENSION ----------------
//void UART2_DMA_Init(void) {
//    MCLK->AHBMASK.bit.DMAC_ = 1;
//    MCLK->APBBMASK.bit.DMAC_ = 1;
//
//    DMAC->BASEADDR.reg = (uint32_t)dmaDescriptor;
//    DMAC->WRBADDR.reg  = (uint32_t)dmaWriteback;
//    DMAC->CTRL.reg     = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xF);
//
//    DMAC->CHID.reg = BOARD_UART_DMA_CH;
//    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(SERCOM2_DMAC_ID_TX) |
//                        DMAC_CHCTRLA_TRIGACT_BEAT;
//
//    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
//    NVIC_EnableIRQ(DMAC_0_IRQn + BOARD_UART_DMA_CH); // maps to correct IRQ
//}
//
//bool UART2_DMA_Send(const char *buffer, uint32_t length) {
//    if (uart2_dma_busy) {
//        return false; // busy, did not start
//    }
//
//    uart2_dma_busy = true;
//
//    dmaDescriptor[BOARD_UART_DMA_CH].BTCTRL.reg = DMAC_BTCTRL_VALID |
//                                  DMAC_BTCTRL_SRCINC |
//                                  DMAC_BTCTRL_BEATSIZE_BYTE |
//                                  DMAC_BTCTRL_BLOCKACT_INT;
//    dmaDescriptor[BOARD_UART_DMA_CH].BTCNT.reg  = length;
//    dmaDescriptor[BOARD_UART_DMA_CH].SRCADDR.reg = (uint32_t)(buffer + length);
//    dmaDescriptor[BOARD_UART_DMA_CH].DSTADDR.reg = (uint32_t)&SERCOM2->USART.DATA.reg;
//    dmaDescriptor[BOARD_UART_DMA_CH].DESCADDR.reg = 0;
//
//    DMAC->CHID.reg = BOARD_UART_DMA_CH;
//    DMAC->CHCTRLA.bit.ENABLE = 1;
//
//    return true; // successfully started
//}
//
//void DMAC_0_Handler(void) {
//    if (DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) {
//        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL; // clear
//        uart2_dma_busy = false;
//        if (uart2_dma_callback) {
//            uart2_dma_callback(); // notify user
//        }
//    }
//}
//
//bool UART2_DMA_IsBusy(void) {
//    return uart2_dma_busy;
//}
//
//void UART2_DMA_SetCallback(UART2_DMA_Callback_t cb) {
//    uart2_dma_callback = cb;
//}
