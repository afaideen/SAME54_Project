#include "board.h"

void SystemConfigPerformance(void) {
	NVMCTRL->CTRLA.bit.RWS = 5;
	CMCC->CTRL.bit.CEN = 1;

	OSCCTRL->XOSCCTRL[0].reg = OSCCTRL_XOSCCTRL_ENABLE |
	                           OSCCTRL_XOSCCTRL_XTALEN |
	                           OSCCTRL_XOSCCTRL_IMULT(4) |
	                           OSCCTRL_XOSCCTRL_IPTAT(3);
	while (!(OSCCTRL->STATUS.bit.XOSCRDY0));

	OSCCTRL->Dpll[0].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC0;
	OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDR(9);
	OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;
	while (!(OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK));

	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN;
	while (GCLK->SYNCBUSY.bit.GENCTRL0);

	MCLK->CPUDIV.reg = MCLK_CPUDIV_DIV_DIV1;
}

void BOARD_Init(void) {
//	// Configure LED0 (PC18) as output, initialize OFF (active low)
//	PORT->Group[LED0_PORT].DIRSET.reg = LED0_MASK;
//	PORT->Group[LED0_PORT].OUTSET.reg = LED0_MASK;  // LED off
//
//	// Configure Button SW0 (PB31) as input with pull-up
//	PORT->Group[BUTTON0_PORT].DIRCLR.reg = BUTTON0_MASK;
//	PORT->Group[BUTTON0_PORT].PINCFG[BUTTON0_PIN].bit.INEN = 1;
//	PORT->Group[BUTTON0_PORT].PINCFG[BUTTON0_PIN].bit.PULLEN = 1;
//	PORT->Group[BUTTON0_PORT].OUTSET.reg = BUTTON0_MASK; // enable pull-up

	// LED0: PC18 output, init off (active low)
	TRISCbits.DIRSET = LED0_MASK;  // like TRISCbits.TRISC18 = 0
	LATCSET = LED0_MASK;           // LED off

	// SW0: PB31 input with pull-up
	TRISBbits.DIRCLR = BUTTON0_MASK;
	PORT->Group[BUTTON0_PORT].PINCFG[BUTTON0_PIN].bit.INEN = 1;
	PORT->Group[BUTTON0_PORT].PINCFG[BUTTON0_PIN].bit.PULLEN = 1;
	LATBSET = BUTTON0_MASK;        // pull-up
}

//-----------------------------------------------------------------------------
// LED helpers (PC18, active low)
//-----------------------------------------------------------------------------
void LED_On(void) {
	PORTCbits.OUTCLR = LED0_MASK;  // active low
}

void LED_Off(void) {
	PORTCbits.OUTSET = LED0_MASK;
}

void LED_Toggle(void) {
	PORTCbits.OUTTGL = LED0_MASK;
}

//-----------------------------------------------------------------------------
// Button helper (PB31, active low)
//-----------------------------------------------------------------------------
bool Button_IsPressed(void) {
	return (PORTBINbits.reg & BUTTON0_MASK) == 0; // active low
}



