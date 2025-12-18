/**
 * \file            timer_example_esp_ticks.c
 * \brief           HSM timer example using esp_ticks platform adapter
 * 
 * This example shows how to use esp_ticks as timer backend for HSM.
 * The adapter is in examples/platform_adapters/hsm_platform_esp_ticks.c
 */

#include "hsm.h"
#include "hsm_platform_esp_ticks.h"  /* Platform adapter */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "hsm_example";

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
            if (hsm_timer_create(&timer_blink, hsm, EVT_BLINK_TICK, 
                               500, HSM_TIMER_PERIODIC) == HSM_RES_OK) {
                hsm_timer_start(timer_blink);
                ESP_LOGI(TAG, "[ACTIVE] Blink timer started");
            }

            /* Create and start timeout timer (5s one-shot) */
            if (hsm_timer_create(&timer_timeout, hsm, EVT_AUTO_TIMEOUT, 
                               5000, HSM_TIMER_ONE_SHOT) == HSM_RES_OK) {
                hsm_timer_start(timer_timeout);
                ESP_LOGI(TAG, "[ACTIVE] Auto-timeout timer started (5s)");
            }
            break;

        case HSM_EVENT_EXIT:
            ESP_LOGI(TAG, "[ACTIVE] Exit - Cleanup timers");
            hsm_timer_delete(timer_blink);
            hsm_timer_delete(timer_timeout);
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

    ESP_LOGI(TAG, "=== HSM with esp_ticks Platform Adapter ===");
    ESP_LOGI(TAG, "HSM core is portable - this uses optional esp_ticks backend\n");

    /* Initialize platform timer (esp_ticks) */
    if (hsm_platform_esp_ticks_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize platform timer");
        return;
    }

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, NULL);

    /* Initialize HSM with platform-specific timer interface */
    hsm_init(&led_hsm, "LED_HSM", &state_idle, hsm_platform_esp_ticks_get_if());

    /* Test sequence */
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "\n--- Test 1: Start and wait for auto-timeout ---");
    hsm_dispatch(&led_hsm, EVT_START, NULL);

    /* Wait for auto-timeout (5 seconds) */
    vTaskDelay(pdMS_TO_TICKS(6000));

    /* Manual start again */
    ESP_LOGI(TAG, "\n--- Test 2: Start and manual stop ---");
    hsm_dispatch(&led_hsm, EVT_START, NULL);

    /* Manual stop after 3 seconds */
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "\n--- Manual stop ---");
    hsm_dispatch(&led_hsm, EVT_STOP, NULL);

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "\n=== Complete ===");

    /* Cleanup */
    hsm_platform_esp_ticks_deinit();
}
