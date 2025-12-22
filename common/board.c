#include "board.h"
#include "systick.h"



void BOARD_Init(void) {

	// LED0: PC18 output, init OFF (active low)
    PORT_DIRSET(LED0_PORT, LED0_MASK);
    LED0_Off();

    // SW0: PB31 input with pull-up (active low)
    PORT_DIRCLR(BUTTON0_PORT, BUTTON0_MASK);

    // Enable input buffer + pull enable (no .bit fields in this header model)
    PORT_PINCFG_SET(BUTTON0_PORT, BUTTON0_PIN, PORT_PINCFG_INEN_PULLEN);

    // Select pull-up: when PULLEN=1, OUT=1 means pull-up
    PORT_OUTSET(BUTTON0_PORT, BUTTON0_MASK);
}

bool led0_is_on(void)
{
    /* Active-low LED on PC18 */
    return ((PORT_REGS->GROUP[LED0_PORT].PORT_OUT & (LED0_MASK)) == 0);
}



static sw_state_t sw0_state = {
    .stable_state = false,
    .last_raw = false,
    .press_start_ms = 0
};

bool sw_pressed(sw_id_t sw, uint32_t debounce)
{
    bool raw;
    sw_state_t *s;
    uint32_t now = millis();

    switch (sw) {
        case SW0:
            raw = SW0_Pressed();
            s = &sw0_state;
            break;
        default:
            return false;
    }

    /* detect raw change */
    if (raw != s->last_raw) {
        s->last_raw = raw;

        if (raw) {
            /* press started */
            s->press_start_ms = now;
        } else {
            /* released */
            s->stable_state = false;
        }
    }

    /* qualify press duration */
    if (raw && !s->stable_state) {
        if ((now - s->press_start_ms) >= debounce) {
            s->stable_state = true;
        }
    }

    return s->stable_state;
}



