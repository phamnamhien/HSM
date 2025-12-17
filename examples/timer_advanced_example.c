/**
 * \file            timer_advanced_example.c
 * \brief           Advanced timer example showing one-shot and multiple events
 */

#include "hsm.h"
#include <stdio.h>

/* Custom events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_BUTTON_PRESS,
    EVT_DEBOUNCE_DONE,     /* One-shot timer for debounce */
    EVT_AUTO_TIMEOUT,      /* One-shot timer for auto-off */
    EVT_BLINK_TICK,        /* Periodic timer for blinking */
} app_events_t;

/* States */
static hsm_state_t state_idle;
static hsm_state_t state_debouncing;
static hsm_state_t state_active;

/**
 * \brief           IDLE state - waiting for button
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Waiting for button press...\n");
            break;

        case EVT_BUTTON_PRESS:
            printf("[IDLE] Button pressed! Start debouncing...\n");
            hsm_transition(hsm, &state_debouncing, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

/**
 * \brief           DEBOUNCING state - wait 50ms for stable signal
 */
static hsm_event_t
debouncing_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[DEBOUNCING] Start 50ms one-shot timer\n");
            /* One-shot timer: fires once after 50ms */
            hsm_timer_start(hsm, EVT_DEBOUNCE_DONE, 50, HSM_TIMER_ONE_SHOT);
            break;

        case HSM_EVENT_EXIT:
            printf("[DEBOUNCING] Exit\n");
            /* Timer auto-stops if transition happens before timeout */
            break;

        case EVT_DEBOUNCE_DONE:
            /* Fired once after 50ms */
            printf("[DEBOUNCING] Debounce complete! Moving to active state\n");
            hsm_transition(hsm, &state_active, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            /* Ignore additional presses during debounce */
            printf("[DEBOUNCING] Ignoring button press (debouncing)\n");
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

/**
 * \brief           ACTIVE state - device is on, with auto-off and blinking
 */
static hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t blink_count = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[ACTIVE] Device ON\n");
            blink_count = 0;

            /* Start periodic blinking every 500ms */
            hsm_timer_start(hsm, EVT_BLINK_TICK, 500, HSM_TIMER_PERIODIC);

            /* NOTE: Can't have 2 timers at once!
             * If you need multiple timers, call hsm_timer_start() again
             * after handling first event, or use external timer manager
             */
            break;

        case HSM_EVENT_EXIT:
            printf("[ACTIVE] Device OFF\n");
            break;

        case EVT_BLINK_TICK:
            /* Fires every 500ms */
            blink_count++;
            printf("[ACTIVE] Blink tick %d\n", blink_count);

            /* After 10 blinks (5 seconds), start auto-off timer */
            if (blink_count >= 10) {
                printf("[ACTIVE] 10 blinks done, starting 3s auto-off timer\n");
                /* Switch to one-shot timer for auto-off */
                hsm_timer_start(hsm, EVT_AUTO_TIMEOUT, 3000, HSM_TIMER_ONE_SHOT);
            }
            return HSM_EVENT_NONE;

        case EVT_AUTO_TIMEOUT:
            /* Fires once after 3000ms */
            printf("[ACTIVE] Auto-off timeout! Going to idle\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            /* Manual turn off */
            printf("[ACTIVE] Button pressed - manual turn off\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            /* Timer automatically stops here! */
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

void
app_main(void) {
    hsm_t device_hsm;

    printf("=== HSM Advanced Timer Example ===\n");
    printf("Shows: one-shot timer, periodic timer, timer auto-stop\n\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_debouncing, "DEBOUNCING", debouncing_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, NULL);

    /* Initialize HSM (timer_if = NULL for this example) */
    hsm_init(&device_hsm, "DeviceHSM", &state_idle, NULL);

    printf("\n--- Simulating button press ---\n");
    hsm_dispatch(&device_hsm, EVT_BUTTON_PRESS, NULL);

    /* Simulate 50ms delay */
    printf("\n[Waiting 50ms...]\n");
    /* In real app: vTaskDelay() or HAL_Delay() */

    /* Manually trigger debounce timeout for demo */
    hsm_dispatch(&device_hsm, EVT_DEBOUNCE_DONE, NULL);

    /* Simulate periodic blink ticks */
    printf("\n--- Simulating blink ticks ---\n");
    for (int i = 0; i < 11; i++) {
        printf("\n[Tick %d]\n", i + 1);
        hsm_dispatch(&device_hsm, EVT_BLINK_TICK, NULL);
    }

    /* Simulate auto-off timeout */
    printf("\n[Waiting 3000ms for auto-off...]\n");
    hsm_dispatch(&device_hsm, EVT_AUTO_TIMEOUT, NULL);

    printf("\n=== Example Complete ===\n");
    printf("\nKey points:\n");
    printf("1. Custom events (EVT_DEBOUNCE_DONE, EVT_BLINK_TICK, etc)\n");
    printf("2. One-shot timer (HSM_TIMER_ONE_SHOT)\n");
    printf("3. Periodic timer (HSM_TIMER_PERIODIC)\n");
    printf("4. Timer auto-stops when state changes\n");
    printf("5. Can switch timer mode in same state\n");
}
