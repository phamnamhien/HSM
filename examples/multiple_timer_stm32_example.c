/**
 * \file            multiple_timer_stm32_example.c
 * \brief           Multiple timer example for STM32 - LED blink + timeout
 */

#include "hsm.h"
#include "main.h"
#include <stdio.h>

/* Events */
typedef enum {
    EVT_SET_BAUD = HSM_EVENT_USER,
    EVT_BAUD_CHANGED,
    EVT_BAUD_INVALID,
    EVT_LED_TICK,
    EVT_SETTING_TIMEOUT,
    EVT_GO_RUN,
} app_event_t;

/* States */
static hsm_state_t state_setting_baudrate;
static hsm_state_t state_run;

/* Timers */
static hsm_timer_t *timer_led_blink;
static hsm_timer_t *timer_setting_timeout;

/* Config */
#define SETTING_LED_BLINK_MS        500
#define SETTING_TIMEOUT_MS          5000

/* App data */
typedef struct {
    hsm_t hsm;
    uint32_t modbus_address;
    uint32_t baudrate;
} app_hsm_t;

/* Platform timer */
typedef struct {
    TIM_HandleTypeDef* htim;
    void (*callback)(void*);
    void* arg;
    uint32_t period_ms;
    uint32_t counter;
    uint8_t repeat;
    uint8_t active;
} stm32_timer_t;

static stm32_timer_t hsm_timers[HSM_CFG_MAX_TIMERS];

/**
 * \brief           Timer IRQ handler - call this from HAL_TIM_PeriodElapsedCallback
 *                  TIM3 must be configured for 1ms period:
 *                  - Prescaler = (APB_Clock / 1000000) - 1  (e.g., 72-1 for 72MHz)
 *                  - ARR = 1000 - 1 = 999
 */
void hsm_timer_irq_handler(void) {
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (hsm_timers[i].active) {
            hsm_timers[i].counter++;  // Count every 1ms
            
            if (hsm_timers[i].counter >= hsm_timers[i].period_ms) {
                hsm_timers[i].counter = 0;
                
                if (hsm_timers[i].callback) {
                    hsm_timers[i].callback(hsm_timers[i].arg);
                }
                
                if (!hsm_timers[i].repeat) {
                    hsm_timers[i].active = 0;
                }
            }
        }
    }
}

void* stm32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    if (!callback || period_ms < 1) return NULL;

    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (!hsm_timers[i].active) {
            hsm_timers[i].htim = &htim3;
            hsm_timers[i].callback = callback;
            hsm_timers[i].arg = arg;
            hsm_timers[i].period_ms = period_ms;
            hsm_timers[i].counter = 0;
            hsm_timers[i].repeat = repeat;
            hsm_timers[i].active = 1;

            // Start TIM3 if not running
            if (HAL_TIM_Base_GetState(&htim3) != HAL_TIM_STATE_BUSY) {
                HAL_TIM_Base_Start_IT(&htim3);
            }

            return &hsm_timers[i];
        }
    }
    return NULL;
}

void stm32_timer_stop(void* timer_handle) {
    stm32_timer_t* timer = (stm32_timer_t*)timer_handle;
    if (timer && timer->active) {
        timer->active = 0;
        
        // Stop TIM3 if no active timers
        uint8_t has_active = 0;
        for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
            if (hsm_timers[i].active) {
                has_active = 1;
                break;
            }
        }
        if (!has_active) {
            HAL_TIM_Base_Stop_IT(&htim3);
        }
    }
}

uint32_t stm32_timer_get_ms(void) {
    return HAL_GetTick();
}

/**
 * \brief           SETTING state handler
 */
static hsm_event_t
state_setting_baudrate_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_hsm_t* me = (app_hsm_t*)hsm;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[SETTING] Entry\n");

            /* LEDs off */
            HAL_GPIO_WritePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_STT_GPIO_Port, LED_STT_Pin, GPIO_PIN_RESET);

            /* LED blink timer */
            hsm_timer_create(&timer_led_blink, hsm, EVT_LED_TICK,
                           SETTING_LED_BLINK_MS, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_led_blink);

            /* Timeout timer */
            hsm_timer_create(&timer_setting_timeout, hsm, EVT_SETTING_TIMEOUT,
                           SETTING_TIMEOUT_MS, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_setting_timeout);

            printf("[SETTING] Timers started\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[SETTING] Exit\n");
            hsm_timer_delete(timer_led_blink);
            hsm_timer_delete(timer_setting_timeout);
            break;

        case EVT_LED_TICK:
            HAL_GPIO_TogglePin(LED_RUN_GPIO_Port, LED_RUN_Pin);
            break;

        case EVT_BAUD_INVALID:
            printf("[SETTING] Invalid baud\n");
            hsm_timer_restart(timer_setting_timeout);
            break;

        case EVT_BAUD_CHANGED:
            printf("[SETTING] Baud changed\n");
            me->baudrate = me->modbus_address;
            hsm_timer_restart(timer_setting_timeout);
            break;

        case EVT_SETTING_TIMEOUT:
            printf("[SETTING] Timeout! -> RUN\n");
            me->baudrate = me->modbus_address;
            HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
            hsm_transition(hsm, &state_run, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           RUN state handler
 */
static hsm_event_t
state_run_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[RUN] Entry\n");
            HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_STT_GPIO_Port, LED_STT_Pin, GPIO_PIN_SET);
            break;

        case HSM_EVENT_EXIT:
            printf("[RUN] Exit\n");
            break;
    }
    return event;
}

/**
 * \brief           Init app HSM
 */
void
app_hsm_init(app_hsm_t* me, uint32_t modbus_address) {
    static const hsm_timer_if_t stm32_timer_if = {
        .start = stm32_timer_start,
        .stop = stm32_timer_stop,
        .get_ms = stm32_timer_get_ms
    };

    /* Create states */
    hsm_state_create(&state_setting_baudrate, "SETTING",
                    state_setting_baudrate_handler, NULL);
    hsm_state_create(&state_run, "RUN", state_run_handler, NULL);

    /* Init data */
    me->modbus_address = modbus_address;
    me->baudrate = 0;

    /* Init HSM */
    if (modbus_address == 0) {
        hsm_init((hsm_t*)me, "APP", &state_setting_baudrate, &stm32_timer_if);
    } else {
        hsm_init((hsm_t*)me, "APP", &state_run, &stm32_timer_if);
    }
}

/**
 * \brief           Main
 */
int
main(void) {
    app_hsm_t app;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM3_Init();

    printf("\n=== Multiple Timer Example ===\n\n");

    /* Init with no address (setting mode) */
    app_hsm_init(&app, 0);

    /* Simulate events */
    HAL_Delay(1000);
    printf("\nBaud change...\n");
    hsm_dispatch((hsm_t*)&app, EVT_BAUD_CHANGED, NULL);

    HAL_Delay(2000);
    printf("\nInvalid baud...\n");
    hsm_dispatch((hsm_t*)&app, EVT_BAUD_INVALID, NULL);

    printf("\nWaiting timeout...\n");

    while (1) {
        HAL_Delay(100);
    }

    return 0;
}

/**
 * \brief           TIM3 interrupt
 */
void
HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        hsm_timer_irq_handler();
    }
}