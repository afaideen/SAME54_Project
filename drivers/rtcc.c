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
#include <stdio.h>
#include "rtcc.h"

#include "rtcc.h"


/* ---------- BCD helpers ---------- */

static inline uint8_t bin2bcd(uint8_t v)
{
    return (uint8_t)(((v / 10) << 4) | (v % 10));
}

static inline uint8_t bcd2bin(uint8_t v)
{
    return (uint8_t)(((v >> 4) * 10) + (v & 0x0F));
}

/* ---------- RTC Calendar ---------- */
void RTC_Clock_32kHz_Init(void)
{
    /* Enable clocks for OSC32KCTRL + RTC */
    MCLK_REGS->MCLK_APBAMASK |= (MCLK_APBAMASK_OSC32KCTRL_Msk | MCLK_APBAMASK_RTC_Msk);

    /* Enable external 32.768 kHz crystal */
    OSC32KCTRL_REGS->OSC32KCTRL_XOSC32K =
        OSC32KCTRL_XOSC32K_ENABLE_Msk |
        OSC32KCTRL_XOSC32K_XTALEN_Msk |
        OSC32KCTRL_XOSC32K_EN32K_Msk |
        OSC32KCTRL_XOSC32K_RUNSTDBY_Msk |
        OSC32KCTRL_XOSC32K_STARTUP_CYCLE32768;

    /* Run continuously (not on-demand) */
    OSC32KCTRL_REGS->OSC32KCTRL_XOSC32K &= (uint16_t)~OSC32KCTRL_XOSC32K_ONDEMAND_Msk;

    while ((OSC32KCTRL_REGS->OSC32KCTRL_STATUS & OSC32KCTRL_STATUS_XOSC32KRDY_Msk) == 0U)
    {
        /* wait for XOSC32K ready */
    }

    /* Select XOSC32K as RTC clock source */
    OSC32KCTRL_REGS->OSC32KCTRL_RTCCTRL = OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K;
}

void RTC_CalendarInit(void)
{
    /* 1) Bring up / select 32k source for RTC (bundled as you requested) */
    RTC_Clock_32kHz_Init();

    /* 2) Enable RTC bus clock (safe even if already enabled) */
    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_RTC_Msk;

    /* 3) Reset RTC (MODE2 calendar) */
    RTC_REGS->MODE2.RTC_CTRLA = RTC_MODE2_CTRLA_SWRST_Msk;
    while (RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_SWRST_Msk) {}

    /* 4) Disable before configuration */
    RTC_REGS->MODE2.RTC_CTRLA &= (uint16_t)~RTC_MODE2_CTRLA_ENABLE_Msk;
    while (RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_ENABLE_Msk) {}

    /* 5) Wait for clock sync to stop (won't hang now because RTCSEL is set) */
    while (RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_CLOCK_Msk) {}

    /* 6) Configure: MODE2 clock + prescaler */
    RTC_REGS->MODE2.RTC_CTRLA =
        (uint16_t)(RTC_MODE2_CTRLA_MODE_CLOCK |
                   RTC_MODE2_CTRLA_PRESCALER_DIV1024);

    /* 7) Enable RTC */
    RTC_REGS->MODE2.RTC_CTRLA |= RTC_MODE2_CTRLA_ENABLE_Msk;
    while (RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_ENABLE_Msk) {}
}

void RTC_CalendarSet(const rtc_time_t *t)
{
    uint32_t clk = 0;

    clk |= RTC_MODE2_CLOCK_SECOND(bin2bcd(t->sec));
    clk |= RTC_MODE2_CLOCK_MINUTE(bin2bcd(t->min));
    clk |= RTC_MODE2_CLOCK_HOUR(bin2bcd(t->hour));
    clk |= RTC_MODE2_CLOCK_DAY(bin2bcd(t->day));
    clk |= RTC_MODE2_CLOCK_MONTH(bin2bcd(t->month));
    clk |= RTC_MODE2_CLOCK_YEAR(bin2bcd(t->year));

    RTC_REGS->MODE2.RTC_CLOCK = clk;
}

void RTC_CalendarGet(rtc_time_t *t)
{
    uint32_t clk = RTC_REGS->MODE2.RTC_CLOCK;

    t->sec   = bcd2bin((clk & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos);
    t->min   = bcd2bin((clk & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos);
    t->hour  = bcd2bin((clk & RTC_MODE2_CLOCK_HOUR_Msk)   >> RTC_MODE2_CLOCK_HOUR_Pos);
    t->day   = bcd2bin((clk & RTC_MODE2_CLOCK_DAY_Msk)    >> RTC_MODE2_CLOCK_DAY_Pos);
    t->month = bcd2bin((clk & RTC_MODE2_CLOCK_MONTH_Msk)  >> RTC_MODE2_CLOCK_MONTH_Pos);
    t->year  = bcd2bin((clk & RTC_MODE2_CLOCK_YEAR_Msk)   >> RTC_MODE2_CLOCK_YEAR_Pos);
}

uint32_t RTC_CalendarRaw(void)
{
    return RTC_REGS->MODE2.RTC_CLOCK;
}



