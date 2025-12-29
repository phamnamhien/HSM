/**
 * \file            transition_param_example.c
 * \brief           Transition parameter and method hook example
 */

#include "hsm.h"
#include <stdio.h>
#include <stdint.h>

/* Transition data */
typedef struct {
    uint32_t error_code;
    uint32_t retry_count;
    const char* message;
} transition_data_t;

/* States */
static hsm_state_t state_idle;
static hsm_state_t state_connecting;
static hsm_state_t state_connected;
static hsm_state_t state_error;

/* Events */
typedef enum {
    EVT_CONNECT = HSM_EVENT_USER,
    EVT_SUCCESS,
    EVT_FAIL,
    EVT_DISCONNECT,
    EVT_RETRY,
} app_events_t;

/**
 * \brief           Transition hook
 */
static void
transition_hook(hsm_t* hsm, void* param) {
    transition_data_t* data = (transition_data_t*)param;

    printf("\n>>> TRANSITION HOOK <<<\n");

    if (data) {
        printf("  Error code: %u\n", data->error_code);
        printf("  Retry count: %u\n", data->retry_count);
        printf("  Message: %s\n", data->message);
        printf("  Cleanup...\n");
    }

    printf(">>> HOOK COMPLETE <<<\n\n");
}

/**
 * \brief           IDLE state
 */
static hsm_event_t
idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[IDLE] Entry\n");
            if (data) {
                transition_data_t* td = (transition_data_t*)data;
                printf("[IDLE] Data: %s\n", td->message);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[IDLE] Exit\n");
            break;

        case EVT_CONNECT: {
            printf("[IDLE] Connect\n");
            transition_data_t td = {0, 0, "Starting connection"};
            hsm_transition(hsm, &state_connecting, &td, NULL);
            return HSM_EVENT_NONE;
        }
    }
    return event;
}

/**
 * \brief           CONNECTING state
 */
static hsm_event_t
connecting_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    static uint32_t retry_count = 0;

    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[CONNECTING] Entry\n");
            if (data) {
                transition_data_t* td = (transition_data_t*)data;
                retry_count = td->retry_count;
                printf("[CONNECTING] Retry: %u\n", retry_count);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[CONNECTING] Exit\n");
            break;

        case EVT_SUCCESS: {
            printf("[CONNECTING] Success!\n");
            transition_data_t td = {0, retry_count, "Connected"};
            hsm_transition(hsm, &state_connected, &td, transition_hook);
            return HSM_EVENT_NONE;
        }

        case EVT_FAIL: {
            printf("[CONNECTING] Failed!\n");
            retry_count++;

            if (retry_count < 3) {
                printf("[CONNECTING] Retry %u\n", retry_count + 1);
                transition_data_t td = {1001, retry_count, "Retrying"};
                hsm_transition(hsm, &state_connecting, &td, NULL);
            } else {
                printf("[CONNECTING] Max retries\n");
                transition_data_t td = {1002, retry_count, "Failed after 3 attempts"};
                hsm_transition(hsm, &state_error, &td, transition_hook);
            }
            return HSM_EVENT_NONE;
        }
    }
    return event;
}

/**
 * \brief           CONNECTED state
 */
static hsm_event_t
connected_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[CONNECTED] Entry\n");
            if (data) {
                transition_data_t* td = (transition_data_t*)data;
                printf("[CONNECTED] Success after %u retries\n", td->retry_count);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[CONNECTED] Exit\n");
            break;

        case EVT_DISCONNECT: {
            printf("[CONNECTED] Disconnect\n");
            transition_data_t td = {0, 0, "User disconnect"};
            hsm_transition(hsm, &state_idle, &td, transition_hook);
            return HSM_EVENT_NONE;
        }
    }
    return event;
}

/**
 * \brief           ERROR state
 */
static hsm_event_t
error_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[ERROR] Entry\n");
            if (data) {
                transition_data_t* td = (transition_data_t*)data;
                printf("[ERROR] Code: %u\n", td->error_code);
                printf("[ERROR] Message: %s\n", td->message);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[ERROR] Exit\n");
            break;

        case EVT_RETRY: {
            printf("[ERROR] Retry\n");
            transition_data_t td = {0, 0, "Manual retry"};
            hsm_transition(hsm, &state_idle, &td, NULL);
            return HSM_EVENT_NONE;
        }
    }
    return event;
}

int
main(void) {
    hsm_t conn_hsm;

    printf("=== Transition Param & Method Example ===\n\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_connecting, "CONNECTING", connecting_handler, NULL);
    hsm_state_create(&state_connected, "CONNECTED", connected_handler, NULL);
    hsm_state_create(&state_error, "ERROR", error_handler, NULL);

    /* Init */
    hsm_init(&conn_hsm, "ConnectionHSM", &state_idle);

    /* Test 1: Success */
    printf("\n--- Test 1: Successful connection ---\n");
    hsm_dispatch(&conn_hsm, EVT_CONNECT, NULL);
    hsm_dispatch(&conn_hsm, EVT_SUCCESS, NULL);

    /* Test 2: Disconnect */
    printf("\n--- Test 2: Disconnect ---\n");
    hsm_dispatch(&conn_hsm, EVT_DISCONNECT, NULL);

    /* Test 3: Failed with retries */
    printf("\n--- Test 3: Failed connection ---\n");
    hsm_dispatch(&conn_hsm, EVT_CONNECT, NULL);
    hsm_dispatch(&conn_hsm, EVT_FAIL, NULL);
    hsm_dispatch(&conn_hsm, EVT_FAIL, NULL);
    hsm_dispatch(&conn_hsm, EVT_FAIL, NULL);

    /* Test 4: Recover */
    printf("\n--- Test 4: Recover ---\n");
    hsm_dispatch(&conn_hsm, EVT_RETRY, NULL);

    printf("\n=== Complete ===\n");

    return 0;
}
