/*
 * Real-Time Clock/Calendar (RTCC) bare‑metal driver for SAME54
 *
 * This module provides a minimalist interface to configure and use the
 * RTC peripheral in MODE2 (calendar clock) using CMSIS/DFP register
 * definitions.  It assumes that the 32.768 kHz crystal (XOSC32K) has been
 * enabled and routed to Generic Clock Generator 1, and that GCLK1 is
 * connected to the RTC peripheral (e.g. via SystemConfigPerformance()).
 *
 * All time/date fields are handled in binary form; internal registers use
 * packed BCD, so the driver performs BCD conversion automatically.
 *
 * Copyright (c) 2025 Han.  All rights reserved.
 */

#ifndef RTC_CALENDAR_H
#define RTC_CALENDAR_H

#include <stdint.h>
#include "sam.h"
#include "instance/rtc.h"

/* Calendar time structure (binary, not BCD) */
typedef struct
{
    uint8_t sec;    /* 0?59 */
    uint8_t min;    /* 0?59 */
    uint8_t hour;   /* 0?23 */
    uint8_t day;    /* 1?31 */
    uint8_t month;  /* 1?12 */
    uint8_t year;   /* 0?99 (2000?2099) */
} rtc_time_t;

/* Initialize RTC in MODE2 (calendar clock) */
void RTC_CalendarInit(void);
void RTC_Clock_32kHz_Init(void);

/* Set calendar time/date (binary input) */
void RTC_CalendarSet(const rtc_time_t *t);

/* Get calendar time/date (binary output) */
void RTC_CalendarGet(rtc_time_t *t);

/* Read raw MODE2 CLOCK register (BCD packed) */
uint32_t RTC_CalendarRaw(void);

#endif /* RTC_CALENDAR_H */

