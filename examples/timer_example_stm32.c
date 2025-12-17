/**
 * \file            timer_example_stm32.c
 * \brief           Multiple timer example for STM32 HAL
 */

#include "hsm.h"
#include "main.h"
#include <stdio.h>

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
extern TIM_HandleTypeDef htim3;

/**
 * \brief           Timer IRQ handler - call this from HAL_TIM_PeriodElapsedCallback
 *                  TIM3 must be configured for 1ms period (Prescaler = APB_CLK/1MHz - 1, ARR = 999)
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

static void* stm32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
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

static void stm32_timer_stop(void* timer_handle) {
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

static uint32_t stm32_timer_get_ms(void) {
    return HAL_GetTick();
}

static const hsm_timer_if_t stm32_timer_if = {
    .start = stm32_timer_start,
    .stop = stm32_timer_stop,
    .get_ms = stm32_timer_get_ms
};

/* States */
static hsm_state_t state_idle;
static hsm_state_t state_running;

/* Timers */
static hsm_timer_t *timer_led;
static hsm_timer_t *timer_watchdog;

/* Events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_STOP,
    EVT_LED_TOGGLE,
    EVT_WATCHDOG,
} app_events_t;

static hsm_event_t idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Entry\n");
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            break;

        case EVT_START:
            printf("[IDLE] Start\n");
            hsm_transition(hsm, &state_running, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

static hsm_event_t running_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t toggle = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[RUNNING] Entry\n");
            toggle = 0;

            /* LED timer */
            hsm_timer_create(&timer_led, hsm, EVT_LED_TOGGLE, 500, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_led);

            /* Watchdog */
            hsm_timer_create(&timer_watchdog, hsm, EVT_WATCHDOG, 10000, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_watchdog);
            break;

        case HSM_EVENT_EXIT:
            printf("[RUNNING] Exit\n");
            hsm_timer_delete(timer_led);
            hsm_timer_delete(timer_watchdog);
            break;

        case EVT_LED_TOGGLE:
            toggle = !toggle;
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, toggle ? GPIO_PIN_SET : GPIO_PIN_RESET);
            printf("[RUNNING] LED %s\n", toggle ? "ON" : "OFF");
            break;

        case EVT_WATCHDOG:
            printf("[RUNNING] Watchdog timeout!\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_STOP:
            printf("[RUNNING] Stop\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) {
        hsm_timer_irq_handler();
    }
}

int main(void) {
    hsm_t my_hsm;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM3_Init();

    printf("=== Multiple Timer Example ===\n");

    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_running, "RUNNING", running_handler, NULL);

    hsm_init(&my_hsm, "STM32_HSM", &state_idle, &stm32_timer_if);

    HAL_Delay(1000);
    hsm_dispatch(&my_hsm, EVT_START, NULL);

    while (1) {
        HAL_Delay(100);
    }
}