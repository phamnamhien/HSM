/**
 * \file            transition_param_example.c
 * \brief           Example showing param and method usage in transitions
 */

#include "hsm.h"
#include <stdio.h>
#include <stdint.h>

/* Custom data structure to pass between states */
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
 * \brief           Transition hook - called between EXIT and ENTRY
 *                  Used for cleanup, logging, synchronization, etc.
 */
static void
transition_hook(hsm_t* hsm, void* param) {
    transition_data_t* data = (transition_data_t*)param;
    
    printf("\n>>> TRANSITION HOOK CALLED <<<\n");
    
    if (data != NULL) {
        printf("  Error code: %u\n", data->error_code);
        printf("  Retry count: %u\n", data->retry_count);
        printf("  Message: %s\n", data->message);
        
        /* Simulate cleanup operations */
        printf("  Performing cleanup...\n");
        printf("  Logging to database...\n");
        printf("  Syncing state...\n");
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
            if (data != NULL) {
                transition_data_t* td = (transition_data_t*)data;
                printf("[IDLE] Received data: %s\n", td->message);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[IDLE] Exit\n");
            break;

        case EVT_CONNECT: {
            printf("[IDLE] Connect request\n");
            transition_data_t td = {
                .error_code = 0,
                .retry_count = 0,
                .message = "Starting connection"
            };
            /* Transition with data */
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
            if (data != NULL) {
                transition_data_t* td = (transition_data_t*)data;
                retry_count = td->retry_count;
                printf("[CONNECTING] Retry count: %u\n", retry_count);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[CONNECTING] Exit\n");
            break;

        case EVT_SUCCESS: {
            printf("[CONNECTING] Connection successful!\n");
            transition_data_t td = {
                .error_code = 0,
                .retry_count = retry_count,
                .message = "Connected successfully"
            };
            /* Transition with data and hook */
            hsm_transition(hsm, &state_connected, &td, transition_hook);
            return HSM_EVENT_NONE;
        }

        case EVT_FAIL: {
            printf("[CONNECTING] Connection failed!\n");
            retry_count++;
            
            if (retry_count < 3) {
                printf("[CONNECTING] Retrying... (attempt %u)\n", retry_count + 1);
                transition_data_t td = {
                    .error_code = 1001,
                    .retry_count = retry_count,
                    .message = "Retrying connection"
                };
                /* Retry with updated data */
                hsm_transition(hsm, &state_connecting, &td, NULL);
            } else {
                printf("[CONNECTING] Max retries exceeded\n");
                transition_data_t td = {
                    .error_code = 1002,
                    .retry_count = retry_count,
                    .message = "Connection failed after 3 attempts"
                };
                /* Go to error state with data and hook */
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
            printf("[CONNECTED] Entry - Online!\n");
            if (data != NULL) {
                transition_data_t* td = (transition_data_t*)data;
                printf("[CONNECTED] Success after %u retries\n", td->retry_count);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[CONNECTED] Exit\n");
            break;

        case EVT_DISCONNECT: {
            printf("[CONNECTED] Disconnecting...\n");
            transition_data_t td = {
                .error_code = 0,
                .retry_count = 0,
                .message = "User requested disconnect"
            };
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
            printf("[ERROR] Entry - System Error!\n");
            if (data != NULL) {
                transition_data_t* td = (transition_data_t*)data;
                printf("[ERROR] Error code: %u\n", td->error_code);
                printf("[ERROR] Message: %s\n", td->message);
            }
            break;

        case HSM_EVENT_EXIT:
            printf("[ERROR] Exit\n");
            break;

        case EVT_RETRY: {
            printf("[ERROR] Retrying from scratch...\n");
            transition_data_t td = {
                .error_code = 0,
                .retry_count = 0,
                .message = "Manual retry initiated"
            };
            hsm_transition(hsm, &state_idle, &td, NULL);
            return HSM_EVENT_NONE;
        }
    }
    return event;
}

int
main(void) {
    hsm_t conn_hsm;

    printf("=== HSM Transition Param & Method Example ===\n\n");

    /* Create states */
    hsm_state_create(&state_idle, "IDLE", idle_handler, NULL);
    hsm_state_create(&state_connecting, "CONNECTING", connecting_handler, NULL);
    hsm_state_create(&state_connected, "CONNECTED", connected_handler, NULL);
    hsm_state_create(&state_error, "ERROR", error_handler, NULL);

    /* Initialize HSM */
    hsm_init(&conn_hsm, "ConnectionHSM", &state_idle, NULL);

    printf("\n--- Test 1: Successful connection ---\n");
    hsm_dispatch(&conn_hsm, EVT_CONNECT, NULL);
    hsm_dispatch(&conn_hsm, EVT_SUCCESS, NULL);

    printf("\n--- Test 2: Disconnect ---\n");
    hsm_dispatch(&conn_hsm, EVT_DISCONNECT, NULL);

    printf("\n--- Test 3: Failed connection with retries ---\n");
    hsm_dispatch(&conn_hsm, EVT_CONNECT, NULL);
    hsm_dispatch(&conn_hsm, EVT_FAIL, NULL);  // Retry 1
    hsm_dispatch(&conn_hsm, EVT_FAIL, NULL);  // Retry 2
    hsm_dispatch(&conn_hsm, EVT_FAIL, NULL);  // Retry 3 -> Error

    printf("\n--- Test 4: Recover from error ---\n");
    hsm_dispatch(&conn_hsm, EVT_RETRY, NULL);

    printf("\n=== Example Complete ===\n");
    printf("\nKey concepts demonstrated:\n");
    printf("1. Passing data via 'param' to ENTRY/EXIT handlers\n");
    printf("2. Using 'method' hook for cleanup between EXIT and ENTRY\n");
    printf("3. Tracking state across transitions (retry_count)\n");
    printf("4. Conditional transitions based on data\n");

    return 0;
}
