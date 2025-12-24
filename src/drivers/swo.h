#ifndef SWO_H
#define SWO_H

#include "sam.h"
#include <stdint.h>

void SWO_Init(uint32_t cpu_freq, uint32_t swo_freq);
void SWO_PrintChar(char c);
void SWO_PrintString(const char *s);

#endif
