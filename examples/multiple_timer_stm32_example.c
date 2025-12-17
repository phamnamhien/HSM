/**
 * \file            multiple_timer_stm32_example.c
 * \brief           Example showing multiple timer usage for STM32
 *                  - LED blink timer (periodic)
 *                  - Setting timeout timer (one-shot)
 */

#include "hsm.h"
#include "main.h" /* STM32 HAL */
#include <stdio.h>

/* Events */
typedef enum {
    EVT_SET_BAUD = HSM_EVENT_USER,
    EVT_BAUD_CHANGED,
    EVT_BAUD_INVALID,
    EVT_LED_TICK,                  /* LED blink event */
    EVT_SETTING_TIMEOUT,           /* Setting done timeout */
    EVT_GO_RUN,
} app_event_t;

/* HSM states */
static hsm_state_t state_setting_baudrate;
static hsm_state_t state_run;

/* Timers */
static hsm_timer_t *timer_led_blink;
static hsm_timer_t *timer_setting_timeout;

/* Application data */
typedef struct {
    hsm_t hsm;              /* Must be first member for casting */
    uint32_t modbus_address;
    uint32_t baudrate;
} app_hsm_t;

/* Timer configuration */
#define HSM_TIM                     (&htim3)
#define SETTING_LED_BLINK_MS        500
#define SETTING_TIMEOUT_MS          5000

/* Platform timer implementation */
typedef struct {
    TIM_HandleTypeDef* htim;
    void (*callback)(void*);
    void* arg;
    uint32_t period_ms;
    uint8_t repeat;
    uint8_t active;
} stm32_timer_t;

static stm32_timer_t hsm_timers[HSM_CFG_MAX_TIMERS];

/**
 * \brief           Timer IRQ handler - called from HAL_TIM_PeriodElapsedCallback
 */
void
hsm_timer_irq_handler(void) {
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (hsm_timers[i].active && hsm_timers[i].callback != NULL) {
            hsm_timers[i].callback(hsm_timers[i].arg);

            /* Stop if one-shot */
            if (!hsm_timers[i].repeat) {
                HAL_TIM_Base_Stop_IT(hsm_timers[i].htim);
                hsm_timers[i].active = 0;
            }
        }
    }
}

void*
stm32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    if (callback == NULL || period_ms < 1) {
        return NULL;
    }

    /* Find free slot */
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (!hsm_timers[i].active) {
            /* Setup timer */
            hsm_timers[i].htim = HSM_TIM;
            hsm_timers[i].callback = callback;
            hsm_timers[i].arg = arg;
            hsm_timers[i].period_ms = period_ms;
            hsm_timers[i].repeat = repeat;
            hsm_timers[i].active = 1;

            /* Configure timer (assuming 1ms tick) */
            __HAL_TIM_SET_AUTORELOAD(hsm_timers[i].htim, period_ms - 1);
            __HAL_TIM_SET_COUNTER(hsm_timers[i].htim, 0);

            /* Start timer interrupt */
            HAL_TIM_Base_Start_IT(hsm_timers[i].htim);

            return &hsm_timers[i];
        }
    }

    return NULL; /* No free slot */
}

void
stm32_timer_stop(void* timer_handle) {
    stm32_timer_t* timer = (stm32_timer_t*)timer_handle;
    
    if (timer != NULL && timer->active) {
        HAL_TIM_Base_Stop_IT(timer->htim);
        timer->active = 0;
    }
}

uint32_t
stm32_timer_get_ms(void) {
    return HAL_GetTick();
}

/**
 * \brief           SETTING BAUDRATE state handler
 */
static hsm_event_t
state_setting_baudrate_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    app_hsm_t* me = (app_hsm_t*)hsm;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[SETTING] Entry\n");
            
            /* Turn off all LEDs */
            HAL_GPIO_WritePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_STT_GPIO_Port, LED_STT_Pin, GPIO_PIN_RESET);
            
            /* Create and start LED blink timer (periodic) */
            hsm_timer_create(&timer_led_blink, hsm, EVT_LED_TICK, 
                           SETTING_LED_BLINK_MS, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_led_blink);
            
            /* Create and start setting timeout timer (one-shot) */
            hsm_timer_create(&timer_setting_timeout, hsm, EVT_SETTING_TIMEOUT,
                           SETTING_TIMEOUT_MS, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_setting_timeout);
            
            printf("[SETTING] LED blink and timeout timers started\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[SETTING] Exit\n");
            /* Cleanup timers */
            hsm_timer_delete(timer_led_blink);
            hsm_timer_delete(timer_setting_timeout);
            break;

        case EVT_LED_TICK:
            /* Toggle LED RUN periodically */
            HAL_GPIO_TogglePin(LED_RUN_GPIO_Port, LED_RUN_Pin);
            break;

        case EVT_BAUD_INVALID:
            printf("[SETTING] Invalid baud, restarting timeout\n");
            /* Restart the timeout timer */
            hsm_timer_restart(timer_setting_timeout);
            break;

        case EVT_BAUD_CHANGED:
            printf("[SETTING] Baud changed, restarting timeout\n");
            me->baudrate = me->modbus_address;
            /* Restart the timeout timer */
            hsm_timer_restart(timer_setting_timeout);
            break;

        case EVT_SETTING_TIMEOUT:
            printf("[SETTING] Timeout! Saving and transitioning to RUN\n");
            
            /* Save to EEPROM */
            me->baudrate = me->modbus_address;
            // EE_Write();
            
            /* Turn on RUN LED solid */
            HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
            
            /* Transition to RUN state */
            hsm_transition(hsm, &state_run, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            return event;
    }
    return HSM_EVENT_NONE;
}

/**
 * \brief           RUN state handler
 */
static hsm_event_t
state_run_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[RUN] Entry - System running\n");
            HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_STT_GPIO_Port, LED_STT_Pin, GPIO_PIN_SET);
            break;

        case HSM_EVENT_EXIT:
            printf("[RUN] Exit\n");
            break;

        default:
            return event;
    }
    return HSM_EVENT_NONE;
}

/**
 * \brief           Initialize application HSM
 */
void
app_hsm_init(app_hsm_t* me, uint32_t modbus_address) {
    /* Timer interface */
    static const hsm_timer_if_t stm32_timer_if = {
        .start = stm32_timer_start,
        .stop = stm32_timer_stop,
        .get_ms = stm32_timer_get_ms
    };

    /* Create states */
    hsm_state_create(&state_setting_baudrate, "SETTING", 
                    state_setting_baudrate_handler, NULL);
    hsm_state_create(&state_run, "RUN", state_run_handler, NULL);

    /* Initialize data */
    me->modbus_address = modbus_address;
    me->baudrate = 0;

    /* Initialize HSM */
    if (modbus_address == 0) {
        /* No address set, go to setting mode */
        hsm_init((hsm_t*)me, "APP", &state_setting_baudrate, &stm32_timer_if);
    } else {
        /* Address already set, go directly to run */
        hsm_init((hsm_t*)me, "APP", &state_run, &stm32_timer_if);
    }
}

/**
 * \brief           Main application
 */
int
main(void) {
    app_hsm_t app;

    /* HAL Init */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM3_Init();

    printf("\n=== Multiple Timer HSM Example ===\n\n");

    /* Initialize application with no modbus address (setting mode) */
    app_hsm_init(&app, 0);

    /* Simulate baud rate changes */
    HAL_Delay(1000);
    printf("\nSimulating baud change event...\n");
    hsm_dispatch((hsm_t*)&app, EVT_BAUD_CHANGED, NULL);

    HAL_Delay(2000);
    printf("\nSimulating invalid baud event...\n");
    hsm_dispatch((hsm_t*)&app, EVT_BAUD_INVALID, NULL);

    /* Wait for timeout to occur (5 seconds total) */
    printf("\nWaiting for timeout...\n");

    /* Main loop */
    while (1) {
        /* Process events */
        HAL_Delay(100);
    }

    return 0;
}

/**
 * \brief           TIM3 interrupt handler
 */
void
HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        hsm_timer_irq_handler();
    }
}
