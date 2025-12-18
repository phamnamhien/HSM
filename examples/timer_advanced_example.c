/**
 * \file            timer_advanced_example.c
 * \brief           Advanced multiple timer example
 * \version         1.0.1
 * 
 * This example demonstrates:
 * - Multiple timers per state
 * - One-shot and periodic timers
 * - Safe timer usage without manual deletion in EXIT handlers
 */

#include "hsm.h"
#include <stdio.h>

/* Events */
typedef enum {
    EVT_BUTTON_PRESS = HSM_EVENT_USER,
    EVT_DEBOUNCE_DONE,
    EVT_BLINK_TICK,
    EVT_AUTO_OFF,
} app_events_t;

/* States */
static hsm_state_t state_idle;
static hsm_state_t state_debouncing;
static hsm_state_t state_active;

/* Timers */
static hsm_timer_t *timer_debounce;
static hsm_timer_t *timer_blink;
static hsm_timer_t *timer_auto_off;

/**
 * \brief           IDLE state handler
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Waiting for button press...\n");
            break;

        case EVT_BUTTON_PRESS:
            printf("[IDLE] Button pressed -> Debouncing\n");
            hsm_transition(hsm, &state_debouncing, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           DEBOUNCING state - one-shot timer
 */
static hsm_event_t
debouncing_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[DEBOUNCING] Starting 50ms debounce timer\n");
            
            /* Create and start one-shot timer */
            hsm_timer_create(&timer_debounce, hsm, EVT_DEBOUNCE_DONE, 
                           50, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_debounce);
            break;

        case HSM_EVENT_EXIT:
            printf("[DEBOUNCING] Exit\n");
            /* Timer auto-deleted by HSM - no manual cleanup needed */
            break;

        case EVT_DEBOUNCE_DONE:
            printf("[DEBOUNCING] Debounce complete -> Active\n");
            hsm_transition(hsm, &state_active, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[DEBOUNCING] Ignoring spurious button press\n");
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           ACTIVE state - multiple timers
 */
static hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t blink_count = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[ACTIVE] Device ON\n");
            blink_count = 0;

            /* Create blink timer (periodic, 500ms) */
            hsm_timer_create(&timer_blink, hsm, EVT_BLINK_TICK, 
                           500, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_blink);
            printf("[ACTIVE] Blink timer started (500ms periodic)\n");

            /* Create auto-off timer (one-shot, 5 seconds) */
            hsm_timer_create(&timer_auto_off, hsm, EVT_AUTO_OFF, 
                           5000, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_auto_off);
            printf("[ACTIVE] Auto-off timer started (5s one-shot)\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[ACTIVE] Device OFF\n");
            /* v1.0.1: Timers automatically deleted by HSM */
            printf("[ACTIVE] Timers cleaned up automatically\n");
            break;

        case EVT_BLINK_TICK:
            blink_count++;
            printf("[ACTIVE] Blink #%d (LED toggle)\n", blink_count);
            break;

        case EVT_AUTO_OFF:
            printf("[ACTIVE] Auto-off timeout reached!\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[ACTIVE] Manual button press -> OFF\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           Main function
 */
int
main(void) {
    hsm_t device_hsm;

    printf("=== HSM v1.0.1 - Advanced Multiple Timer Example ===\n");
    printf("Features demonstrated:\n");
    printf("1. One-shot timer (debounce)\n");
    printf("2. Periodic timer (blink)\n");
    printf("3. Multiple concurrent timers\n");
    printf("4. Automatic timer cleanup (v1.0.1 fix)\n");
    printf("5. No race conditions!\n\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_debouncing, "DEBOUNCING", debouncing_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, NULL);

    /* Initialize HSM */
    hsm_init(&device_hsm, "DeviceHSM", &state_idle, NULL);

    /* Simulate button press */
    printf("\n--- Simulating button press ---\n");
    hsm_dispatch(&device_hsm, EVT_BUTTON_PRESS, NULL);

    /* Simulate debounce complete */
    printf("\n[Waiting 50ms for debounce...]\n");
    hsm_dispatch(&device_hsm, EVT_DEBOUNCE_DONE, NULL);

    /* Simulate blink events */
    printf("\n--- Simulating blink ticks ---\n");
    for (int i = 0; i < 3; i++) {
        printf("\n[500ms later...]\n");
        hsm_dispatch(&device_hsm, EVT_BLINK_TICK, NULL);
    }

    /* Simulate auto-off */
    printf("\n[5 seconds later...]\n");
    printf("--- Auto-off timer fires ---\n");
    hsm_dispatch(&device_hsm, EVT_AUTO_OFF, NULL);

    printf("\n=== Complete ===\n");
    printf("Note: In v1.0.1, timers are automatically deleted on transition\n");
    printf("No manual cleanup needed in EXIT handlers!\n");

    return 0;
}
