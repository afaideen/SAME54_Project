
#include <stdio.h>
#include <time.h>
#include "board.h"
#include "systick.h"
#include "delay.h"
#include "cpu.h"
#include "../drivers/uart_dma.h"
#include "../drivers/rtcc.h"
#include "../drivers/qspi/qspi_flash.h"
#include "../drivers/qspi/qspi_hw.h"

/* Provided by your SysTick code */
extern uint32_t millis(void);


void FW_LogBanner(void)
{
    struct tm t;

    printf("\r\n================ FIRMWARE INFO ================\r\n");
    printf("Name           : %s\r\n", FW_NAME);
    printf("Version        : %s\r\n", FW_VERSION);
    printf("Build Date     : %s\r\n", __DATE__);
    printf("Build Time     : %s\r\n", __TIME__);
    char rtcc_str[32];
    if (RTCC_FormatDateTime(rtcc_str, sizeof(rtcc_str))) {
        printf("RTC Time       : %s\r\n", rtcc_str);
    } else {
        printf("RTC Time       : NOT VALID\r\n");
    }

    printf("Boot Time      : %lu ms since reset\r\n", millis());
    printf("===============================================\r\n\r\n");
}


/**
 * Initializes board peripherals including LED and button
 * Configures LED0 (PC18) as output (active low) and SW0 (PB31) as input with pull-up
 */
void board_init(void) 
{    
    
	// LED0: PC18 output, init OFF (active low)
    PORT_DIRSET(LED0_PORT, LED0_MASK);
    board_led0_off();

    // SW0: PB31 input with pull-up (active low)
    PORT_DIRCLR(BUTTON0_PORT, BUTTON0_MASK);

    // Enable input buffer + pull enable (no .bit fields in this header model)
    PORT_PINCFG_SET(BUTTON0_PORT, BUTTON0_PIN, PORT_PINCFG_INEN_PULLEN);

    // Select pull-up: when PULLEN=1, OUT=1 means pull-up
    PORT_OUTSET(BUTTON0_PORT, BUTTON0_MASK);
    
    /* Initialize SysTick for 1ms ticks */
    SysTick_Init_1ms_Best(BOARD_CPU_CLOCK, true);

    /* Initialize the UART peripheral (SERCOM2) first, then DMA */    
    UART2_DMA_Init();
    #ifdef BOARD_ENABLE_RTCC
        RTCC_Init();
        /* Sync RTCC once */
        RTCC_SyncFromBuildTime();
    #endif
    #ifdef USE_QSPI_FLASH
        QSPI_HW_PinInit();
        if (!QSPI_Flash_Init())
        {
            printf("[QSPI] Init FAILED (JEDEC mismatch or bus issue)\r\n");
        }
        
    #endif    
    CPU_LogClockOverview();
    QSPI_Flash_Diag_Print();
    FW_LogBanner();  
}

/**
 * Checks if LED0 is currently ON
 * @return true if LED0 is on, false otherwise (LED0 is active low)
 */
bool board_led0_is_on(void)
{
    /* Active-low LED on PC18 */
    return ((PORT_REGS->GROUP[LED0_PORT].PORT_OUT & (LED0_MASK)) == 0);
}

/**
 * Turns LED0 ON
 * LED0 is active low, so this drives PC18 low
 */
void board_led0_on(void)
{
    LED0_On();
}

/**
 * Turns LED0 OFF
 * LED0 is active low, so this drives PC18 high
 */
void board_led0_off(void)
{
    LED0_Off();
}

/**
 * Toggles LED0 state
 */
void board_led0_toggle(void)
{
    LED0_Toggle();
}

/**
 * Detects if a switch is pressed with debouncing
 * @param sw Switch ID to check
 * @param debounce Debounce time in milliseconds
 * @return true if switch is stably pressed for at least debounce duration, false otherwise
 */
//bool board_sw_pressed(sw_id_t sw, uint32_t debounce)
bool board_sw_pressed(sw_t *sw)
{
    static sw_state_t sw0_state = {
        .stable_state = false,
        .last_raw = false,
        .press_start_ms = 0
    };
    bool raw;
    sw_state_t *s;
    uint32_t now = millis();

    switch (sw->id) {
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
        if ((now - s->press_start_ms) >= sw->t_debounce) {
            s->stable_state = true;
            sw->cnt++;
            UART2_DMA_Log("\r\nSW0 %s (>=%lu ms)\r\n",
                          SW0_Pressed() ? "PRESSED" : "RELEASED",
                          sw->t_debounce);

            DelayMs(10);
            while (SW0_Pressed()); /* Block until sw released */
        }
    }

    return s->stable_state;
}



