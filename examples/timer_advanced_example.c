/**
 * \file            timer_advanced_example.c
 * \brief           Advanced multiple timer example showing various features
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
 * \brief           IDLE state
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Waiting for button...\n");
            break;

        case EVT_BUTTON_PRESS:
            printf("[IDLE] Button pressed! Debouncing...\n");
            hsm_transition(hsm, &state_debouncing, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           DEBOUNCING state - 50ms one-shot timer
 */
static hsm_event_t
debouncing_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[DEBOUNCING] Start 50ms one-shot timer\n");
            
            /* Create and start debounce timer */
            if (hsm_timer_create(&timer_debounce, hsm, EVT_DEBOUNCE_DONE,
                               50, HSM_TIMER_ONE_SHOT) == HSM_RES_OK) {
                hsm_timer_start(timer_debounce);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[DEBOUNCING] Exit\n");
            hsm_timer_delete(timer_debounce);
            break;

        case EVT_DEBOUNCE_DONE:
            printf("[DEBOUNCING] Debounce complete!\n");
            hsm_transition(hsm, &state_active, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[DEBOUNCING] Ignoring extra presses\n");
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           ACTIVE state - multiple timers demo
 */
static hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint8_t blink_count = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[ACTIVE] Device ON - Starting multiple timers\n");
            blink_count = 0;
            
            /* Timer 1: LED blink (periodic) */
            if (hsm_timer_create(&timer_blink, hsm, EVT_BLINK_TICK,
                               500, HSM_TIMER_PERIODIC) == HSM_RES_OK) {
                hsm_timer_start(timer_blink);
                printf("[ACTIVE] Blink timer started (500ms periodic)\n");
            }
            
            /* Timer 2: Auto-off (one-shot) */
            if (hsm_timer_create(&timer_auto_off, hsm, EVT_AUTO_OFF,
                               5000, HSM_TIMER_ONE_SHOT) == HSM_RES_OK) {
                hsm_timer_start(timer_auto_off);
                printf("[ACTIVE] Auto-off timer started (5s one-shot)\n");
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[ACTIVE] Device OFF - Cleanup timers\n");
            hsm_timer_delete(timer_blink);
            hsm_timer_delete(timer_auto_off);
            break;

        case EVT_BLINK_TICK:
            blink_count++;
            printf("[ACTIVE] Blink #%d\n", blink_count);
            break;

        case EVT_AUTO_OFF:
            printf("[ACTIVE] Auto-off triggered!\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[ACTIVE] Button pressed - manual off\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

void
app_main(void) {
    hsm_t device_hsm;

    printf("=== HSM Advanced Multiple Timer Example ===\n\n");
    printf("Features demonstrated:\n");
    printf("1. One-shot timer (debounce)\n");
    printf("2. Periodic timer (blink)\n");
    printf("3. Multiple timers per state\n");
    printf("4. Timer auto-stop on transition\n");
    printf("5. Manual timer cleanup\n\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_debouncing, "DEBOUNCING", debouncing_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, NULL);

    /* Initialize (NULL timer_if for demo) */
    hsm_init(&device_hsm, "DeviceHSM", &state_idle, NULL);

    printf("\n--- Simulating button press ---\n");
    hsm_dispatch(&device_hsm, EVT_BUTTON_PRESS, NULL);

    printf("\n[Simulating 50ms delay...]\n");
    hsm_dispatch(&device_hsm, EVT_DEBOUNCE_DONE, NULL);

    printf("\n--- Simulating blink ticks ---\n");
    for (int i = 0; i < 3; i++) {
        printf("\n[Tick...]\n");
        hsm_dispatch(&device_hsm, EVT_BLINK_TICK, NULL);
    }

    printf("\n[Simulating 5s auto-off...]\n");
    hsm_dispatch(&device_hsm, EVT_AUTO_OFF, NULL);

    printf("\n=== Example Complete ===\n");
}
