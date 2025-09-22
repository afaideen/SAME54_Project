#include "systick.h"
#include "delay.h"

bool DelayMs(uint32_t *t1, unsigned int ms) {
    if (*t1 == 0) {
        *t1 = systick_ms;
    }
    if ((systick_ms - *t1) >= ms) {
        *t1 = 0;
        return true;
    }
    return false;
}
