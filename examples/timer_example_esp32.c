/**
 * \file            timer_example_esp32.c
 * \brief           HSM timer example for ESP32
 */

#include "hsm.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"

static const char* TAG = "HSM_TIMER";

/* Timer interface for ESP32 FreeRTOS */
typedef struct {
    TimerHandle_t handle;
    void (*callback)(void*);
    void* arg;
} esp32_timer_t;

static void
esp32_timer_callback(TimerHandle_t xTimer) {
    esp32_timer_t* timer = (esp32_timer_t*)pvTimerGetTimerID(xTimer);
    if (timer != NULL && timer->callback != NULL) {
        timer->callback(timer->arg);
    }
}

static void*
esp32_timer_start(void (*callback)(void*), void* arg, uint32_t period_ms, uint8_t repeat) {
    esp32_timer_t* timer = malloc(sizeof(esp32_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    timer->callback = callback;
    timer->arg = arg;

    timer->handle = xTimerCreate("hsm_timer", pdMS_TO_TICKS(period_ms), repeat ? pdTRUE : pdFALSE,
                                  timer, esp32_timer_callback);

    if (timer->handle == NULL) {
        free(timer);
        return NULL;
    }

    if (xTimerStart(timer->handle, 0) != pdPASS) {
        xTimerDelete(timer->handle, 0);
        free(timer);
        return NULL;
    }

    return timer;
}

static void
esp32_timer_stop(void* timer_handle) {
    esp32_timer_t* timer = (esp32_timer_t*)timer_handle;
    if (timer != NULL) {
        xTimerStop(timer->handle, 0);
        xTimerDelete(timer->handle, 0);
        free(timer);
    }
}

static uint32_t
esp32_timer_get_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static const hsm_timer_if_t esp32_timer_if = {.start = esp32_timer_start,
                                               .stop = esp32_timer_stop,
                                               .get_ms = esp32_timer_get_ms};

/* HSM states */
static hsm_state_t state_idle;
static hsm_state_t state_blinking;
static hsm_state_t state_fast_blink;

/* Events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_STOP,
    EVT_SPEED_UP,
    EVT_BLINK_TIMEOUT,     /* Custom timeout event for blinking */
    EVT_FAST_TIMEOUT,      /* Custom timeout event for fast blink */
} app_events_t;

/**
 * \brief           IDLE state - LED off
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[IDLE] LED OFF");
            /* Turn off LED here */
            break;

        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "[IDLE] Exit");
            break;

        case EVT_START:
            ESP_LOGI(TAG, "[IDLE] Start blinking");
            hsm_transition(hsm, &state_blinking, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

/**
 * \brief           BLINKING state - LED blinks every 1000ms
 */
static hsm_event_t
blinking_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t led_state = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[BLINKING] Entry - Start 1000ms periodic timer");
            led_state = 0;
            /* Start periodic timer with custom event */
            hsm_timer_start(hsm, EVT_BLINK_TIMEOUT, 1000, HSM_TIMER_PERIODIC);
            
            /* NOTE: If you call transition here, timer will auto-stop! */
            // hsm_transition(hsm, &state_fast_blink, NULL, NULL); // Timer stops automatically
            break;

        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "[BLINKING] Exit - Timer auto-stopped");
            /* Timer is automatically stopped when leaving state */
            break;

        case EVT_BLINK_TIMEOUT:
            /* Called every 1000ms - custom event! */
            led_state = !led_state;
            ESP_LOGI(TAG, "[BLINKING] Custom timeout - LED %s", led_state ? "ON" : "OFF");
            /* Toggle LED here */
            break;

        case EVT_STOP:
            ESP_LOGI(TAG, "[BLINKING] Stop");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_SPEED_UP:
            ESP_LOGI(TAG, "[BLINKING] Speed up!");
            hsm_transition(hsm, &state_fast_blink, NULL, NULL);
            /* Timer automatically stops here! */
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

/**
 * \brief           FAST_BLINK state - LED blinks every 200ms
 */
static hsm_event_t
fast_blink_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t led_state = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            ESP_LOGI(TAG, "[FAST_BLINK] Entry - Start 200ms periodic timer");
            led_state = 0;
            /* Start periodic timer with different custom event */
            hsm_timer_start(hsm, EVT_FAST_TIMEOUT, 200, HSM_TIMER_PERIODIC);
            break;

        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "[FAST_BLINK] Exit - Timer auto-stopped");
            break;

        case EVT_FAST_TIMEOUT:
            /* Called every 200ms - different custom event! */
            led_state = !led_state;
            ESP_LOGI(TAG, "[FAST_BLINK] Fast timeout - LED %s", led_state ? "ON" : "OFF");
            break;

        case EVT_STOP:
            ESP_LOGI(TAG, "[FAST_BLINK] Stop");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

void
app_main(void) {
    hsm_t led_hsm;

    ESP_LOGI(TAG, "=== HSM Timer Example for ESP32 ===");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_blinking, "BLINKING", blinking_handler, NULL);
    hsm_state_create(&state_fast_blink, "FAST_BLINK", fast_blink_handler, NULL);

    /* Initialize HSM with timer interface */
    hsm_init(&led_hsm, "LED_HSM", &state_idle, &esp32_timer_if);

    ESP_LOGI(TAG, "\n--- Test sequence ---");

    /* Test sequence */
    vTaskDelay(pdMS_TO_TICKS(1000));
    hsm_dispatch(&led_hsm, EVT_START, NULL);

    vTaskDelay(pdMS_TO_TICKS(3500)); /* Let it blink 3 times */

    hsm_dispatch(&led_hsm, EVT_SPEED_UP, NULL);

    vTaskDelay(pdMS_TO_TICKS(2000)); /* Fast blink for 2 seconds */

    hsm_dispatch(&led_hsm, EVT_STOP, NULL);

    ESP_LOGI(TAG, "=== Example Complete ===");
}
