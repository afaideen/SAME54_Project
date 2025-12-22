#ifndef DELAY_H
#define DELAY_H

#include <stdbool.h>
#include <stdint.h>

bool DelayMsAsync(uint32_t *t1, unsigned int ms);
void DelayMs(uint32_t delay_ms);

#endif
