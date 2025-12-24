#include "swo.h"

void SWO_Init(uint32_t cpu_freq, uint32_t swo_freq) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR  = 0xC5ACCE55;
    ITM->TCR  = ITM_TCR_ITMENA_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk;
    ITM->TER  = 0xFFFFFFFF;
    TPI->SPPR = 0x2;
    TPI->ACPR = (cpu_freq / swo_freq) - 1;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void SWO_PrintChar(char c) {
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) && (ITM->TER & 1)) {
        while (ITM->PORT[0].u32 == 0);
        ITM->PORT[0].u8 = (uint8_t)c;
    }
}

void SWO_PrintString(const char *s) {
    while (*s) SWO_PrintChar(*s++);
}
