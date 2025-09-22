#include <stdio.h>
#include "uart.h"
#include "../common/board.h"

__attribute__((aligned(16))) DmacDescriptor dmaDescriptor[12];
__attribute__((aligned(16))) DmacDescriptor dmaWriteback[12];

static volatile bool uart2_dma_busy = false;
static UART2_DMA_Callback_t uart2_dma_callback = NULL;

void UART2_Init(void) {
    MCLK->APBAMASK.bit.SERCOM2_ = 1;
    GCLK->PCHCTRL[SERCOM2_GCLK_ID_CORE].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;

    PORT->Group[UART_RX_PORT].PMUX[UART_RX_PIN >> 1].bit.PMUXE = UART_RX_PMUX;
    PORT->Group[UART_RX_PORT].PINCFG[UART_RX_PIN].bit.PMUXEN = 1;
    PORT->Group[UART_TX_PORT].PMUX[UART_TX_PIN >> 1].bit.PMUXO = UART_TX_PMUX;
    PORT->Group[UART_TX_PORT].PINCFG[UART_TX_PIN].bit.PMUXEN = 1;

    SERCOM2->USART.CTRLA.bit.SWRST = 1;
    while (SERCOM2->USART.SYNCBUSY.bit.SWRST);

    SERCOM2->USART.CTRLA.reg = SERCOM_USART_CTRLA_MODE_USART_INT_CLK |
                               SERCOM_USART_CTRLA_RXPO(1) |
                               SERCOM_USART_CTRLA_TXPO(0) |
                               SERCOM_USART_CTRLA_DORD;
    SERCOM2->USART.CTRLB.reg = SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_RXEN;
    while (SERCOM2->USART.SYNCBUSY.bit.CTRLB);

    uint64_t br = (uint64_t)65536 * (BOARD_CPU_CLOCK - 16ULL * BOARD_UART_BAUDRATE) / BOARD_CPU_CLOCK;
    SERCOM2->USART.BAUD.reg = (uint16_t)br;

    SERCOM2->USART.CTRLA.bit.ENABLE = 1;
    while (SERCOM2->USART.SYNCBUSY.bit.ENABLE);
}

// Blocking transmit
void UART2_Putc(char c) {
    while (!SERCOM2->USART.INTFLAG.bit.DRE);
    SERCOM2->USART.DATA.reg = c;
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

// ---------------- DMA EXTENSION ----------------
void UART2_DMA_Init(void) {
    MCLK->AHBMASK.bit.DMAC_ = 1;
    MCLK->APBBMASK.bit.DMAC_ = 1;

    DMAC->BASEADDR.reg = (uint32_t)dmaDescriptor;
    DMAC->WRBADDR.reg  = (uint32_t)dmaWriteback;
    DMAC->CTRL.reg     = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xF);

    DMAC->CHID.reg = BOARD_UART_DMA_CH;
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(SERCOM2_DMAC_ID_TX) |
                        DMAC_CHCTRLA_TRIGACT_BEAT;

    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
    NVIC_EnableIRQ(DMAC_0_IRQn + BOARD_UART_DMA_CH); // maps to correct IRQ
}

bool UART2_DMA_Send(const char *buffer, uint32_t length) {
    if (uart2_dma_busy) {
        return false; // busy, did not start
    }

    uart2_dma_busy = true;

    dmaDescriptor[BOARD_UART_DMA_CH].BTCTRL.reg = DMAC_BTCTRL_VALID |
                                  DMAC_BTCTRL_SRCINC |
                                  DMAC_BTCTRL_BEATSIZE_BYTE |
                                  DMAC_BTCTRL_BLOCKACT_INT;
    dmaDescriptor[BOARD_UART_DMA_CH].BTCNT.reg  = length;
    dmaDescriptor[BOARD_UART_DMA_CH].SRCADDR.reg = (uint32_t)(buffer + length);
    dmaDescriptor[BOARD_UART_DMA_CH].DSTADDR.reg = (uint32_t)&SERCOM2->USART.DATA.reg;
    dmaDescriptor[BOARD_UART_DMA_CH].DESCADDR.reg = 0;

    DMAC->CHID.reg = BOARD_UART_DMA_CH;
    DMAC->CHCTRLA.bit.ENABLE = 1;

    return true; // successfully started
}

void DMAC_0_Handler(void) {
    if (DMAC->CHINTFLAG.reg & DMAC_CHINTFLAG_TCMPL) {
        DMAC->CHINTFLAG.reg = DMAC_CHINTFLAG_TCMPL; // clear
        uart2_dma_busy = false;
        if (uart2_dma_callback) {
            uart2_dma_callback(); // notify user
        }
    }
}

bool UART2_DMA_IsBusy(void) {
    return uart2_dma_busy;
}

void UART2_DMA_SetCallback(UART2_DMA_Callback_t cb) {
    uart2_dma_callback = cb;
}
