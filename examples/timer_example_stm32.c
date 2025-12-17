/**
 * \file            timer_example_stm32.c
 * \brief           HSM timer example for STM32 HAL
 */

#include "hsm.h"
#include "main.h"
#include <stdio.h>

/* Timer structure for STM32 */
typedef struct {
    TIM_HandleTypeDef* htim;
    void (*callback)(void*);
    void* arg;
    uint32_t period_ms;
    uint8_t repeat;
    uint8_t active;
} stm32_timer_t;

static stm32_timer_t hsm_timer;
extern TIM_HandleTypeDef htim6; /* Use TIM6 for HSM timer */

/**
 * \brief           Timer interrupt callback (called from HAL_TIM_PeriodElapsedCallback)
 */
void
hsm_timer_irq_handler(void) {
    if (hsm_timer.active && hsm_timer.callback != NULL) {
        hsm_timer.callback(hsm_timer.arg);

        /* Stop if one-shot timer */
        if (!hsm_timer.repeat) {
            HAL_TIM_Base_Stop_IT(hsm_timer.htim);
            hsm_timer.active = 0;
        }
    }
}

static void*
stm32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    if (callback == NULL || period_ms < 1) {
        return NULL;
    }

    /* Stop existing timer */
    if (hsm_timer.active) {
        HAL_TIM_Base_Stop_IT(hsm_timer.htim);
    }

    /* Setup timer */
    hsm_timer.htim = &htim6;
    hsm_timer.callback = callback;
    hsm_timer.arg = arg;
    hsm_timer.period_ms = period_ms;
    hsm_timer.repeat = repeat;
    hsm_timer.active = 1;

    /* Configure timer period
     * Assuming TIM6 is configured with 1ms tick
     * Adjust __HAL_TIM_SET_AUTORELOAD based on your clock config
     */
    __HAL_TIM_SET_AUTORELOAD(hsm_timer.htim, period_ms - 1);
    __HAL_TIM_SET_COUNTER(hsm_timer.htim, 0);

    /* Start timer */
    HAL_TIM_Base_Start_IT(hsm_timer.htim);

    return &hsm_timer;
}

static void
stm32_timer_stop(void* timer_handle) {
    if (timer_handle == &hsm_timer && hsm_timer.active) {
        HAL_TIM_Base_Stop_IT(hsm_timer.htim);
        hsm_timer.active = 0;
    }
}

static uint32_t
stm32_timer_get_ms(void) {
    return HAL_GetTick();
}

static const hsm_timer_if_t stm32_timer_if = {.start = stm32_timer_start,
                                               .stop = stm32_timer_stop,
                                               .get_ms = stm32_timer_get_ms};

/* HAL Timer Callback - Add this to stm32xxxx_it.c or main.c */
void
HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM6) {
        hsm_timer_irq_handler();
    }
}

/* HSM states */
static hsm_state_t state_idle;
static hsm_state_t state_running;

/* Events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_STOP,
} app_events_t;

static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Entry\n");
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            break;

        case HSM_EVENT_EXIT:
            printf("[IDLE] Exit\n");
            break;

        case EVT_START:
            printf("[IDLE] Start\n");
            hsm_transition(hsm, &state_running, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

static hsm_event_t
running_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t toggle = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[RUNNING] Entry - Start 500ms repeating timer\n");
            toggle = 0;
            /* Start repeating timer */
            hsm_timer_start(hsm, 500, 1);
            break;

        case HSM_EVENT_EXIT:
            printf("[RUNNING] Exit\n");
            break;

        case HSM_EVENT_TIMEOUT:
            toggle = !toggle;
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, toggle ? GPIO_PIN_SET : GPIO_PIN_RESET);
            printf("[RUNNING] Timeout - LED %s\n", toggle ? "ON" : "OFF");
            break;

        case EVT_STOP:
            printf("[RUNNING] Stop\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

int
main(void) {
    hsm_t my_hsm;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM6_Init();

    printf("=== HSM Timer Example for STM32 ===\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_running, "RUNNING", running_handler, NULL);

    /* Initialize HSM with STM32 timer interface */
    hsm_init(&my_hsm, "STM32_HSM", &state_idle, &stm32_timer_if);

    /* Test sequence */
    HAL_Delay(1000);
    hsm_dispatch(&my_hsm, EVT_START, NULL);

    HAL_Delay(5000);
    hsm_dispatch(&my_hsm, EVT_STOP, NULL);

    while (1) {
    }
}
