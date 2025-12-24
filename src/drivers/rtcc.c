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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "sam.h"
#include "rtcc.h"

/* Guards */
#if !defined(MCLK_REGS) || !defined(OSC32KCTRL_REGS) || !defined(RTC_REGS)
#error "Header model mismatch: expected SAME54 DFP with *_REGS pointers."
#endif

/* Choose RTC clock source:
 * - XOSC1K: uses external 32.768kHz crystal?s 1kHz output (more accurate)
 * - ULP1K : uses internal ultra-low-power 1kHz (works without crystal)
 */
#ifndef RTCC_RTCCTRL_SEL
#define RTCC_RTCCTRL_SEL   OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K
/* #define RTCC_RTCCTRL_SEL OSC32KCTRL_RTCCTRL_RTCSEL_ULP1K */
#endif

/* Option A: RTCC API is only valid after RTCC_Init() is called. */
static bool s_rtcc_inited = false;

static inline void rtc_mode2_wait_sync(uint32_t mask)
{
    while ((RTC_REGS->MODE2.RTC_SYNCBUSY & mask) != 0U) { }
}

static inline uint8_t year_to_hw(uint16_t year)
{
    if (year < 2000U) return 0U;
    if (year > 2063U) return 63U;
    return (uint8_t)(year - 2000U);
}

static inline uint16_t year_from_hw(uint8_t hw_year)
{
    return (uint16_t)(2000U + (uint16_t)(hw_year & 0x3FU));
}

/* ---------- internal helpers ---------- */

static inline uint8_t bcd_to_bin(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t bin_to_bcd(uint8_t bin)
{
    return ((bin / 10) << 4) | (bin % 10);
}

void RTCC_Init(void)
{
    /* Enable bus clocks */
    MCLK_REGS->MCLK_APBAMASK |= (MCLK_APBAMASK_OSC32KCTRL_Msk |
                                MCLK_APBAMASK_RTC_Msk);

    /* If we intend to use XOSC1K, ensure XOSC32K is enabled + EN1K */
    if ((RTCC_RTCCTRL_SEL & OSC32KCTRL_RTCCTRL_RTCSEL_Msk) ==
        OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K)
    {
        uint16_t x = OSC32KCTRL_REGS->OSC32KCTRL_XOSC32K;

        x |= (uint16_t)(OSC32KCTRL_XOSC32K_ENABLE_Msk |
                        OSC32KCTRL_XOSC32K_XTALEN_Msk |
                        OSC32KCTRL_XOSC32K_EN1K_Msk |
                        OSC32KCTRL_XOSC32K_STARTUP_CYCLE32768);

        x &= (uint16_t)~OSC32KCTRL_XOSC32K_ONDEMAND_Msk; /* keep running */

        OSC32KCTRL_REGS->OSC32KCTRL_XOSC32K = x;

        while ((OSC32KCTRL_REGS->OSC32KCTRL_STATUS &
                OSC32KCTRL_STATUS_XOSC32KRDY_Msk) == 0U)
        {
            /* wait */
        }
    }

    /* Select RTC clock source (1kHz domain) */
    OSC32KCTRL_REGS->OSC32KCTRL_RTCCTRL = RTCC_RTCCTRL_SEL;

    /* Disable before reset/config */
    RTC_REGS->MODE2.RTC_CTRLA &= (uint16_t)~RTC_MODE2_CTRLA_ENABLE_Msk;
    while ((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_ENABLE_Msk) != 0U) { }

    /* Software reset */
    RTC_REGS->MODE2.RTC_CTRLA = RTC_MODE2_CTRLA_SWRST_Msk;
    while ((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_SWRST_Msk) != 0U) { }

    /* MODE2 Clock/Calendar, prescaler /1024 => 1 Hz from 1kHz source, stable reads */
    RTC_REGS->MODE2.RTC_CTRLA =
          RTC_MODE2_CTRLA_MODE_CLOCK
        | RTC_MODE2_CTRLA_PRESCALER_DIV1024
        | RTC_MODE2_CTRLA_CLOCKSYNC_Msk;

    /* Enable RTC */
    RTC_REGS->MODE2.RTC_CTRLA |= RTC_MODE2_CTRLA_ENABLE_Msk;
    while ((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_ENABLE_Msk) != 0U) { }
    
    s_rtcc_inited = true;
}
bool RTCC_IsEnabled(void)
{
    return (RTC_REGS->MODE2.RTC_CTRLA & RTC_MODE2_CTRLA_ENABLE_Msk);
}

bool RTCC_TimeGet(struct tm *t)
{
    if (!(RTC_REGS->MODE2.RTC_CTRLA & RTC_MODE2_CTRLA_ENABLE_Msk))
        return false;

    while (RTC_REGS->MODE2.RTC_SYNCBUSY);

    uint32_t clk = RTC_REGS->MODE2.RTC_CLOCK;

    t->tm_sec  = ((RTC_MODE2_CLOCK_SECOND(clk) >> 4) * 10) +
                 (RTC_MODE2_CLOCK_SECOND(clk) & 0xF);

    t->tm_min  = ((RTC_MODE2_CLOCK_MINUTE(clk) >> 4) * 10) +
                 (RTC_MODE2_CLOCK_MINUTE(clk) & 0xF);

    t->tm_hour = ((RTC_MODE2_CLOCK_HOUR(clk) >> 4) * 10) +
                 (RTC_MODE2_CLOCK_HOUR(clk) & 0xF);

    t->tm_mday = ((RTC_MODE2_CLOCK_DAY(clk) >> 4) * 10) +
                 (RTC_MODE2_CLOCK_DAY(clk) & 0xF);

    t->tm_mon  = (((RTC_MODE2_CLOCK_MONTH(clk) >> 4) * 10) +
                 (RTC_MODE2_CLOCK_MONTH(clk) & 0xF)) - 1;

    t->tm_year = (((RTC_MODE2_CLOCK_YEAR(clk) >> 4) * 10) +
                 (RTC_MODE2_CLOCK_YEAR(clk) & 0xF)) + 100;

    return true;
}


void RTCC_TimeSet(const struct tm *t)
{
    while (RTC_REGS->MODE2.RTC_SYNCBUSY);

    RTC_REGS->MODE2.RTC_CTRLA &= ~RTC_MODE2_CTRLA_ENABLE_Msk;
    while (RTC_REGS->MODE2.RTC_SYNCBUSY);

    RTC_REGS->MODE2.RTC_CLOCK =
        RTC_MODE2_CLOCK_SECOND(((t->tm_sec / 10) << 4) | (t->tm_sec % 10)) |
        RTC_MODE2_CLOCK_MINUTE(((t->tm_min / 10) << 4) | (t->tm_min % 10)) |
        RTC_MODE2_CLOCK_HOUR(((t->tm_hour / 10) << 4) | (t->tm_hour % 10)) |
        RTC_MODE2_CLOCK_DAY(((t->tm_mday / 10) << 4) | (t->tm_mday % 10)) |
        RTC_MODE2_CLOCK_MONTH((((t->tm_mon + 1) / 10) << 4) | ((t->tm_mon + 1) % 10)) |
        RTC_MODE2_CLOCK_YEAR((((t->tm_year - 100) / 10) << 4) | ((t->tm_year - 100) % 10));

    RTC_REGS->MODE2.RTC_CTRLA |= RTC_MODE2_CTRLA_ENABLE_Msk;
    while (RTC_REGS->MODE2.RTC_SYNCBUSY);
}

bool RTCC_SetDateTime(const rtcc_datetime_t *dt)
{
    if (!s_rtcc_inited) return false;
    if (dt == NULL) return false;
    if (dt->year  < 2000U || dt->year > 2063U) return false;
    if (dt->month < 1U || dt->month > 12U) return false;
    if (dt->day   < 1U || dt->day   > 31U) return false;
    if (dt->hour  > 23U) return false;
    if (dt->min   > 59U) return false;
    if (dt->sec   > 59U) return false;

    /* Disable before write */
    RTC_REGS->MODE2.RTC_CTRLA &= (uint16_t)~RTC_MODE2_CTRLA_ENABLE_Msk;
    rtc_mode2_wait_sync(RTC_MODE2_SYNCBUSY_ENABLE_Msk);

    /* Write CLOCK fields (binary) */
    RTC_REGS->MODE2.RTC_CLOCK =
          RTC_MODE2_CLOCK_SECOND(dt->sec)
        | RTC_MODE2_CLOCK_MINUTE(dt->min)
        | RTC_MODE2_CLOCK_HOUR(dt->hour)
        | RTC_MODE2_CLOCK_DAY(dt->day)
        | RTC_MODE2_CLOCK_MONTH(dt->month)
        | RTC_MODE2_CLOCK_YEAR(year_to_hw(dt->year));

    rtc_mode2_wait_sync(RTC_MODE2_SYNCBUSY_CLOCK_Msk);

    /* Re-enable */
    RTC_REGS->MODE2.RTC_CTRLA |= RTC_MODE2_CTRLA_ENABLE_Msk;
    rtc_mode2_wait_sync(RTC_MODE2_SYNCBUSY_ENABLE_Msk);

    return true;
}

bool RTCC_GetDateTime(rtcc_datetime_t *dt)
{
    if (!s_rtcc_inited) return false;
    if (dt == NULL) return false;

    /* With CLOCKSYNC enabled: wait until CLOCK is synchronized before reading */
    rtc_mode2_wait_sync(RTC_MODE2_SYNCBUSY_CLOCK_Msk);

    /* Double-read to avoid rollover edge */
    uint32_t c1, c2;
    do {
        c1 = RTC_REGS->MODE2.RTC_CLOCK;
        c2 = RTC_REGS->MODE2.RTC_CLOCK;
    } while (c1 != c2);

    dt->sec   = (uint8_t)((c1 & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos);
    dt->min   = (uint8_t)((c1 & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos);
    dt->hour  = (uint8_t)((c1 & RTC_MODE2_CLOCK_HOUR_Msk)   >> RTC_MODE2_CLOCK_HOUR_Pos);
    dt->day   = (uint8_t)((c1 & RTC_MODE2_CLOCK_DAY_Msk)    >> RTC_MODE2_CLOCK_DAY_Pos);
    dt->month = (uint8_t)((c1 & RTC_MODE2_CLOCK_MONTH_Msk)  >> RTC_MODE2_CLOCK_MONTH_Pos);

    uint8_t y = (uint8_t)((c1 & RTC_MODE2_CLOCK_YEAR_Msk)   >> RTC_MODE2_CLOCK_YEAR_Pos);
    dt->year  = year_from_hw(y);

    /* Reject not-set / invalid values (prevents 2000-00-00 00:00:00) */
    if (dt->month < 1U || dt->month > 12U) return false;
    if (dt->day   < 1U || dt->day   > 31U) return false;
    if (dt->hour  > 23U) return false;
    if (dt->min   > 59U) return false;
    if (dt->sec   > 59U) return false;
    if (dt->year  < 2000U || dt->year > 2063U) return false;

    return true;
}
static uint8_t build_month_to_num(const char *m)
{
    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";

    for (uint8_t i = 0; i < 12; i++)
    {
        if (strncmp(m, &months[i * 3], 3) == 0)
            return i + 1;
    }
    return 1; // fallback
}
static void parse_build_datetime(
    uint16_t *year, uint8_t *month, uint8_t *day,
    uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    /* __DATE__ = "Dec 24 2025" */
    *month = build_month_to_num(__DATE__);
    *day   = (uint8_t)atoi(__DATE__ + 4);
    *year  = (uint16_t)atoi(__DATE__ + 7);

    /* __TIME__ = "08:30:06" */
    *hour = (uint8_t)atoi(__TIME__ + 0);
    *min  = (uint8_t)atoi(__TIME__ + 3);
    *sec  = (uint8_t)atoi(__TIME__ + 6);
}


static int RTCC_IsTimeValid(void)
{
    uint8_t year_bcd =
        (RTC_REGS->MODE2.RTC_CLOCK & RTC_MODE2_CLOCK_YEAR_Msk)
        >> RTC_MODE2_CLOCK_YEAR_Pos;

    uint8_t year =
        ((year_bcd >> 4) * 10) + (year_bcd & 0xF);

    return (year >= 20);   // year >= 2020
}


static bool rtcc_time_plausible(const rtcc_datetime_t *dt)
{
    /* Consider anything >= 2020 as already set (tweak if you want). */
    if (dt == NULL) return false;
    if (dt->year < 2020U || dt->year > 2063U) return false;
    if (dt->month < 1U || dt->month > 12U) return false;
    if (dt->day < 1U || dt->day > 31U) return false;
    if (dt->hour > 23U) return false;
    if (dt->min > 59U) return false;
    if (dt->sec > 59U) return false;
    return true;
}

void RTCC_SyncFromBuildTime(void)
{
    if (!s_rtcc_inited) return;

    /* If RTC already contains a plausible time, do NOT overwrite it. */
    rtcc_datetime_t cur;
    if (RTCC_GetDateTime(&cur) && rtcc_time_plausible(&cur))
    {
        return;
    }

    /* Otherwise seed from firmware build time (__DATE__ / __TIME__). */
    uint16_t year;
    uint8_t month, day, hour, min, sec;
    parse_build_datetime(&year, &month, &day, &hour, &min, &sec);

    rtcc_datetime_t build_dt =
    {
        .year  = year,
        .month = month,
        .day   = day,
        .hour  = hour,
        .min   = min,
        .sec   = sec,
    };

    (void)RTCC_SetDateTime(&build_dt);
}

bool RTCC_FormatDateTime(char *out, uint32_t out_sz)
{
    rtcc_datetime_t dt = {0};

    if ((out == NULL) || (out_sz == 0U)) {
        return false;
    }

    if (!RTCC_GetDateTime(&dt)) {
        out[0] = '\0';
        return false;
    }

    (void)snprintf(out, out_sz, "%04u-%02u-%02u %02u:%02u:%02u",
                   (unsigned)dt.year, (unsigned)dt.month, (unsigned)dt.day,
                   (unsigned)dt.hour, (unsigned)dt.min, (unsigned)dt.sec);
    return true;
}







