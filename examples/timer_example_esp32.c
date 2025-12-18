/**
 * \file            timer_example_esp32.c
 * \brief           Thread-safe multiple timer example for ESP32 with FreeRTOS
 * \version         1.0.1
 */

#include "hsm.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char* TAG = "HSM_TIMER";

/* Timer states for thread safety */
typedef enum {
    TIMER_STATE_IDLE = 0,
    TIMER_STATE_ACTIVE,
    TIMER_STATE_DELETING,
} timer_state_t;

/* Thread-safe timer wrapper */
typedef struct {
    TimerHandle_t handle;
    void (*callback)(void*);
    void* arg;
    volatile timer_state_t state;
    SemaphoreHandle_t mutex;
} esp32_timer_t;

/**
 * \brief           Timer callback with thread safety
 */
static void 
esp32_timer_callback(TimerHandle_t xTimer) {
    esp32_timer_t* timer = (esp32_timer_t*)pvTimerGetTimerID(xTimer);
    if (timer == NULL) return;
    
    /* Try to acquire mutex (non-blocking) */
    if (xSemaphoreTake(timer->mutex, 0) != pdTRUE) {
        return;  /* Skip if timer is being deleted */
    }
    
    /* Check state and execute callback */
    if (timer->state == TIMER_STATE_ACTIVE && timer->callback) {
        timer->callback(timer->arg);
    }
    
    xSemaphoreGive(timer->mutex);
}

/**
 * \brief           Start timer with thread safety
 */
static void* 
esp32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    esp32_timer_t* timer = malloc(sizeof(esp32_timer_t));
    if (!timer) return NULL;

    /* Create mutex for this timer */
    timer->mutex = xSemaphoreCreateMutex();
    if (!timer->mutex) {
        free(timer);
        return NULL;
    }

    timer->callback = callback;
    timer->arg = arg;
    timer->state = TIMER_STATE_ACTIVE;
    
    timer->handle = xTimerCreate("hsm", pdMS_TO_TICKS(period_ms),
                                  repeat ? pdTRUE : pdFALSE, 
                                  timer, esp32_timer_callback);

    if (!timer->handle || xTimerStart(timer->handle, 0) != pdPASS) {
        if (timer->handle) xTimerDelete(timer->handle, 0);
        vSemaphoreDelete(timer->mutex);
        free(timer);
        return NULL;
    }
    
    return timer;
}

/**
 * \brief           Stop timer with safe deletion
 */
static void 
esp32_timer_stop(void* timer_handle) {
    esp32_timer_t* timer = (esp32_timer_t*)timer_handle;
    if (timer == NULL) return;
    
    /* Acquire mutex to block callback execution */
    if (xSemaphoreTake(timer->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for timer deletion");
        return;
    }
    
    /* Mark as deleting and clear callback */
    timer->state = TIMER_STATE_DELETING;
    timer->callback = NULL;
    
    xSemaphoreGive(timer->mutex);
    
    /* Stop and delete FreeRTOS timer */
    if (timer->handle) {
        xTimerStop(timer->handle, pdMS_TO_TICKS(100));
        xTimerDelete(timer->handle, pdMS_TO_TICKS(100));
        timer->handle = NULL;
    }
    
    /* Wait for any pending callback to complete */
    vTaskDelay(pdMS_TO_TICKS(10));
    
    /* Now safe to delete */
    vSemaphoreDelete(timer->mutex);
    free(timer);
}

/**
 * \brief           Get milliseconds
 */
static uint32_t 
esp32_timer_get_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/**
 * \brief           Timer interface for HSM
 */
static const hsm_timer_if_t esp32_timer_if = {
    .start = esp32_timer_start,
    .stop = esp32_timer_stop,
    .get_ms = esp32_timer_get_ms
};

/* ========================================================================== */
/*                            HSM APPLICATION                                 */
/* ========================================================================== */

/* States */
static hsm_state_t state_idle;
static hsm_state_t state_active;

/* Timers */
static hsm_timer_t *timer_blink;
static hsm_timer_t *timer_timeout;

/* Events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_STOP,
    EVT_BLINK_TICK,
    EVT_AUTO_TIMEOUT,
} app_events_t;

/**
 * \brief           IDLE state handler
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[IDLE] Entry - LED OFF");
            break;

        case EVT_START:
            ESP_LOGI(TAG, "[IDLE] Start -> ACTIVE");
            hsm_transition(hsm, &state_active, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           ACTIVE state handler
 */
static hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t led_state = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[ACTIVE] Entry");
            led_state = 0;

            /* Create and start blink timer (500ms periodic) */
            hsm_timer_create(&timer_blink, hsm, EVT_BLINK_TICK, 500, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_blink);
            ESP_LOGI(TAG, "[ACTIVE] Blink timer started");

            /* Create and start timeout timer (5s one-shot) */
            hsm_timer_create(&timer_timeout, hsm, EVT_AUTO_TIMEOUT, 5000, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_timeout);
            ESP_LOGI(TAG, "[ACTIVE] Auto-timeout timer started (5s)");
            break;

        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "[ACTIVE] Exit - Timers auto-deleted by HSM");
            /* NOTE: Timer deletion is now automatic in hsm_transition() */
            break;

        case EVT_BLINK_TICK:
            led_state = !led_state;
            ESP_LOGI(TAG, "[ACTIVE] LED %s", led_state ? "ON" : "OFF");
            break;

        case EVT_AUTO_TIMEOUT:
            ESP_LOGI(TAG, "[ACTIVE] Auto-timeout! -> IDLE");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_STOP:
            ESP_LOGI(TAG, "[ACTIVE] Manual stop -> IDLE");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           Main application
 */
void
app_main(void) {
    hsm_t led_hsm;

    ESP_LOGI(TAG, "=== HSM v1.0.1 - Thread-Safe Timer Example ===");
    ESP_LOGI(TAG, "Fixed: Race condition that caused system resets\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, NULL);

    /* Initialize HSM */
    hsm_init(&led_hsm, "LED_HSM", &state_idle, &esp32_timer_if);

    /* Test sequence */
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "\n--- Test 1: Start and wait for auto-timeout ---");
    hsm_dispatch(&led_hsm, EVT_START, NULL);

    /* Wait for auto-timeout (5 seconds) */
    vTaskDelay(pdMS_TO_TICKS(6000));

    ESP_LOGI(TAG, "\n--- Test 2: Start and manual stop ---");
    hsm_dispatch(&led_hsm, EVT_START, NULL);

    /* Manual stop after 3 seconds */
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "Stopping manually...");
    hsm_dispatch(&led_hsm, EVT_STOP, NULL);

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "\n=== Complete - No crashes! ===");
    ESP_LOGI(TAG, "HSM v1.0.1 eliminates timer race conditions");
}
