/**
 * \file            basic_example.c
 * \brief           Basic HSM usage example
 */

#include "hsm.h"
#include <stdio.h>

/* Events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_STOP,
    EVT_ERROR,
    EVT_RESET,
} app_events_t;

/* States */
static hsm_state_t state_idle;
static hsm_state_t state_running;
static hsm_state_t state_error;

/**
 * \brief           IDLE state handler
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[IDLE] Exit\n");
            break;

        case EVT_START:
            printf("[IDLE] START -> RUNNING\n");
            hsm_transition(hsm, &state_running, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           RUNNING state handler
 */
static hsm_event_t
running_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[RUNNING] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[RUNNING] Exit\n");
            break;

        case EVT_STOP:
            printf("[RUNNING] STOP -> IDLE\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_ERROR:
            printf("[RUNNING] ERROR -> ERROR\n");
            hsm_transition(hsm, &state_error, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           ERROR state handler
 */
static hsm_event_t
error_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[ERROR] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[ERROR] Exit\n");
            break;

        case EVT_RESET:
            printf("[ERROR] RESET -> IDLE\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}

/**
 * \brief           Main
 */
void
app_main(void) {
    hsm_t my_hsm;

    printf("=== HSM Basic Example ===\n\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_running, "RUNNING", running_handler, NULL);
    hsm_state_create(&state_error, "ERROR", error_handler, NULL);

    /* Init HSM */
    hsm_init(&my_hsm, "BasicHSM", &state_idle, NULL);

    /* Test */
    printf("--- Test 1: IDLE -> RUNNING ---\n");
    hsm_dispatch(&my_hsm, EVT_START, NULL);

    printf("\n--- Test 2: RUNNING -> ERROR ---\n");
    hsm_dispatch(&my_hsm, EVT_ERROR, NULL);

    printf("\n--- Test 3: ERROR -> IDLE ---\n");
    hsm_dispatch(&my_hsm, EVT_RESET, NULL);

    printf("\n--- Test 4: IDLE -> RUNNING -> IDLE ---\n");
    hsm_dispatch(&my_hsm, EVT_START, NULL);
    hsm_dispatch(&my_hsm, EVT_STOP, NULL);

    printf("\n=== Complete ===\n");
}