/*
 * Real-Time Clock/Calendar (RTCC) bare‑metal driver implementation.
 *
 * See rtcc.h for high‑level description.  This implementation uses the
 * CMSIS Device Family Pack (DFP) definitions for the SAME54 family and
 * operates directly on the RTC MODE2 registers.
 *
 * It assumes that the clock tree has already been configured such that
 * GCLK1 (driven from the 32.768 kHz crystal) is connected to the RTC
 * peripheral (see SystemConfigPerformance()).
 */
#include <stdarg.h>
#include <stdio.h>
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

    return true;
}






