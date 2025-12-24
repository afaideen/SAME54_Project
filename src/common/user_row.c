#include <stdint.h>

/*
 * SAME54 User Row
 * Mirrors CMSIS default behavior
 * No watchdog, no BOD changes
 */
__attribute__((section(".user_row")))
const uint32_t __user_row[4] =
{
    0xFFFFFFFF,   // USER_WORD_0
    0xFFFFFFFF,   // USER_WORD_1
    0xFFFFFFFF,   // USER_WORD_2
    0xFFFFFFFF    // USER_WORD_3
};

