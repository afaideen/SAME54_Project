/*
 * Real-Time Clock/Calendar (RTCC) bare-metal driver for SAME54
 *
 * This module configures and uses the RTC peripheral in MODE2 (calendar clock)
 * using the SAME54 DFP register model (*_REGS pointers).
 *
 * Clocking:
 * - RTCC_Init() enables OSC32KCTRL + RTC clocks
 * - Optionally enables XOSC32K and selects OSC32KCTRL_RTCCTRL.RTCSEL
 *
 * Date/time fields are handled in binary form (no BCD conversion in this driver).
 */


#ifndef RTCC_H
#define RTCC_H

#include <stdint.h>
#include <stdbool.h>

/* SAME54 RTC MODE2: YEAR field is 0..63 => 2000..2063 */
typedef struct
{
    uint16_t year;   /* full year, e.g. 2025 */
    uint8_t  month;  /* 1..12 */
    uint8_t  day;    /* 1..31 */
    uint8_t  hour;   /* 0..23 */
    uint8_t  min;    /* 0..59 */
    uint8_t  sec;    /* 0..59 */
} rtcc_datetime_t;

/*
 * RTCC driver uses RTC MODE2 (Clock/Calendar).
 * Time/date fields are handled as binary values (RTC_CLOCK fields are binary).
 *
 * RTCC_Init() configures:
 * - APBA clocks for OSC32KCTRL + RTC
 * - selects RTC clock source via OSC32KCTRL_RTCCTRL
 * - resets RTC and configures MODE2 with 1 Hz tick (1 kHz / 1024)
 */
void RTCC_Init(void);
bool RTCC_SetDateTime(const rtcc_datetime_t *dt);
bool RTCC_GetDateTime(rtcc_datetime_t *dt);

#endif




