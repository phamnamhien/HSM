/**
 * \file            timer_example_esp32.c
 * \brief           Multiple timer example for ESP32 with FreeRTOS
 */

#include "hsm.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"

static const char* TAG = "HSM_TIMER";

/* Timer wrapper */
typedef struct {
    TimerHandle_t handle;
    void (*callback)(void*);
    void* arg;
} esp32_timer_t;

static void esp32_timer_callback(TimerHandle_t xTimer) {
    esp32_timer_t* timer = (esp32_timer_t*)pvTimerGetTimerID(xTimer);
    if (timer && timer->callback) {
        timer->callback(timer->arg);
    }
}

static void* esp32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    esp32_timer_t* timer = malloc(sizeof(esp32_timer_t));
    if (!timer) return NULL;

    timer->callback = callback;
    timer->arg = arg;
    timer->handle = xTimerCreate("hsm", pdMS_TO_TICKS(period_ms),
                                  repeat ? pdTRUE : pdFALSE, timer, esp32_timer_callback);

    if (!timer->handle || xTimerStart(timer->handle, 0) != pdPASS) {
        if (timer->handle) xTimerDelete(timer->handle, 0);
        free(timer);
        return NULL;
    }
    return timer;
}

static void esp32_timer_stop(void* timer_handle) {
    esp32_timer_t* timer = (esp32_timer_t*)timer_handle;
    if (timer) {
        xTimerStop(timer->handle, 0);
        xTimerDelete(timer->handle, 0);
        free(timer);
    }
}

static uint32_t esp32_timer_get_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static const hsm_timer_if_t esp32_timer_if = {
    .start = esp32_timer_start,
    .stop = esp32_timer_stop,
    .get_ms = esp32_timer_get_ms
};

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
 * \brief           IDLE state
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[IDLE] LED OFF");
            break;

        case EVT_START:
            ESP_LOGI(TAG, "[IDLE] Start");
            hsm_transition(hsm, &state_active, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           ACTIVE state
 */
static hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t led_state = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[ACTIVE] Entry");
            led_state = 0;

            /* Blink timer (periodic) */
            hsm_timer_create(&timer_blink, hsm, EVT_BLINK_TICK, 500, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_blink);
            ESP_LOGI(TAG, "[ACTIVE] Blink timer started");

            /* Auto-off timer (one-shot) */
            hsm_timer_create(&timer_timeout, hsm, EVT_AUTO_TIMEOUT, 5000, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_timeout);
            ESP_LOGI(TAG, "[ACTIVE] Auto-off timer started (5s)");
            break;

        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "[ACTIVE] Exit - Cleanup");
            hsm_timer_delete(timer_blink);
            hsm_timer_delete(timer_timeout);
            break;

        case EVT_BLINK_TICK:
            led_state = !led_state;
            ESP_LOGI(TAG, "[ACTIVE] LED %s", led_state ? "ON" : "OFF");
            break;

        case EVT_AUTO_TIMEOUT:
            ESP_LOGI(TAG, "[ACTIVE] Auto-off!");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_STOP:
            ESP_LOGI(TAG, "[ACTIVE] Manual stop");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

void
app_main(void) {
    hsm_t led_hsm;

    ESP_LOGI(TAG, "=== Multiple Timer Example ===");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, NULL);

    /* Init HSM */
    hsm_init(&led_hsm, "LED_HSM", &state_idle, &esp32_timer_if);

    /* Test */
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "--- Starting ---");
    hsm_dispatch(&led_hsm, EVT_START, NULL);

    /* Wait for auto-off */
    vTaskDelay(pdMS_TO_TICKS(6000));

    ESP_LOGI(TAG, "=== Complete ===");
}