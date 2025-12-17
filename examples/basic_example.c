/**
 * \file            basic_example.c
 * \brief           Basic HSM usage example
 */

#include "hsm.h"
#include <stdio.h>

/* Define custom events */
typedef enum {
    EVT_START = HSM_EVENT_USER,
    EVT_STOP,
    EVT_ERROR,
    EVT_RESET,
} app_events_t;

/* State structures */
static hsm_state_t state_idle;
static hsm_state_t state_running;
static hsm_state_t state_error;

/**
 * \brief           IDLE state handler
 * \param[in]       hsm: HSM instance
 * \param[in]       event: Event to handle
 * \param[in]       data: Event data
 * \return          Event status
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
            printf("[IDLE] Received START event\n");
            hsm_transition(hsm, &state_running, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            printf("[IDLE] Unhandled event: 0x%lX\n", (unsigned long)event);
            break;
    }
    return event;
}

/**
 * \brief           RUNNING state handler
 * \param[in]       hsm: HSM instance
 * \param[in]       event: Event to handle
 * \param[in]       data: Event data
 * \return          Event status
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
            printf("[RUNNING] Received STOP event\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_ERROR:
            printf("[RUNNING] Received ERROR event\n");
            hsm_transition(hsm, &state_error, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            printf("[RUNNING] Unhandled event: 0x%lX\n", (unsigned long)event);
            break;
    }
    return event;
}

/**
 * \brief           ERROR state handler
 * \param[in]       hsm: HSM instance
 * \param[in]       event: Event to handle
 * \param[in]       data: Event data
 * \return          Event status
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
            printf("[ERROR] Received RESET event\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            printf("[ERROR] Unhandled event: 0x%lX\n", (unsigned long)event);
            break;
    }
    return event;
}

/**
 * \brief           Main application entry
 */
void
app_main(void) {
    hsm_t my_hsm;
    hsm_result_t res;

    printf("=== HSM Basic Example ===\n\n");

    /* Create states */
    res = hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    if (res != HSM_RES_OK) {
        printf("Failed to create IDLE state\n");
        return;
    }

    res = hsm_state_create(&state_running, "RUNNING", running_handler, NULL);
    if (res != HSM_RES_OK) {
        printf("Failed to create RUNNING state\n");
        return;
    }

    res = hsm_state_create(&state_error, "ERROR", error_handler, NULL);
    if (res != HSM_RES_OK) {
        printf("Failed to create ERROR state\n");
        return;
    }

    /* Initialize HSM with IDLE as initial state */
    res = hsm_init(&my_hsm, "BasicHSM", &state_idle);
    if (res != HSM_RES_OK) {
        printf("Failed to initialize HSM\n");
        return;
    }

    printf("\n=== State Machine Initialized ===\n\n");

    /* Test sequence */
    printf("--- Sending START event ---\n");
    hsm_dispatch(&my_hsm, EVT_START, NULL);

    printf("\n--- Sending ERROR event ---\n");
    hsm_dispatch(&my_hsm, EVT_ERROR, NULL);

    printf("\n--- Sending RESET event ---\n");
    hsm_dispatch(&my_hsm, EVT_RESET, NULL);

    printf("\n--- Sending START event ---\n");
    hsm_dispatch(&my_hsm, EVT_START, NULL);

    printf("\n--- Sending STOP event ---\n");
    hsm_dispatch(&my_hsm, EVT_STOP, NULL);

    /* Check current state */
    if (hsm_is_in_state(&my_hsm, &state_idle)) {
        printf("\n=== State Machine is in IDLE state ===\n");
    }

    printf("\n=== Example Complete ===\n");
}
