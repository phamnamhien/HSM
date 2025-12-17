/**
 * \file            hierarchical_example.c
 * \brief           Hierarchical state machine example
 */

#include "hsm.h"
#include <stdio.h>

/* Define custom events */
typedef enum {
    EVT_BUTTON_PRESS = HSM_EVENT_USER,
    EVT_TIMEOUT,
    EVT_MODE_CHANGE,
    EVT_COMMON_ACTION,
} app_events_t;

/* State structures */
static hsm_state_t state_system;        /* Root state */
static hsm_state_t state_active;        /* Parent state */
static hsm_state_t state_mode1;         /* Child state */
static hsm_state_t state_mode2;         /* Child state */
static hsm_state_t state_standby;       /* Sibling state */

/**
 * \brief           SYSTEM root state handler
 */
static hsm_event_t
system_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[SYSTEM] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[SYSTEM] Exit\n");
            break;

        case EVT_COMMON_ACTION:
            printf("[SYSTEM] Common action handled by root\n");
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event;
}

/**
 * \brief           ACTIVE parent state handler
 */
static hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[ACTIVE] Entry (parent state)\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[ACTIVE] Exit (parent state)\n");
            break;

        case EVT_TIMEOUT:
            printf("[ACTIVE] Timeout - going to standby\n");
            hsm_transition(hsm, &state_standby, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event; /* Propagate to parent (SYSTEM) */
}

/**
 * \brief           MODE1 state handler
 */
static hsm_event_t
mode1_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[MODE1] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[MODE1] Exit\n");
            break;

        case EVT_MODE_CHANGE:
            printf("[MODE1] Mode change to MODE2\n");
            hsm_transition(hsm, &state_mode2, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[MODE1] Button pressed\n");
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event; /* Propagate to parent (ACTIVE) */
}

/**
 * \brief           MODE2 state handler
 */
static hsm_event_t
mode2_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[MODE2] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[MODE2] Exit\n");
            break;

        case EVT_MODE_CHANGE:
            printf("[MODE2] Mode change to MODE1\n");
            hsm_transition(hsm, &state_mode1, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[MODE2] Button pressed (different action)\n");
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event; /* Propagate to parent (ACTIVE) */
}

/**
 * \brief           STANDBY state handler
 */
static hsm_event_t
standby_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("[STANDBY] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[STANDBY] Exit\n");
            break;

        case EVT_BUTTON_PRESS:
            printf("[STANDBY] Button pressed - waking up to MODE1\n");
            hsm_transition(hsm, &state_mode1, NULL, NULL);
            return HSM_EVENT_NONE;

        default:
            break;
    }
    return event; /* Propagate to parent (SYSTEM) */
}

/**
 * \brief           Main application entry
 */
void
app_main(void) {
    hsm_t my_hsm;

    printf("=== HSM Hierarchical Example ===\n\n");

    /* Create state hierarchy:
     * SYSTEM (root)
     *   ├── ACTIVE (parent)
     *   │   ├── MODE1 (child)
     *   │   └── MODE2 (child)
     *   └── STANDBY (sibling)
     */

    /* Create root state */
    hsm_state_create(&state_system, "SYSTEM", system_handler, NULL);

    /* Create parent state under root */
    hsm_state_create(&state_active, "ACTIVE", active_handler, &state_system);

    /* Create child states under ACTIVE */
    hsm_state_create(&state_mode1, "MODE1", mode1_handler, &state_active);
    hsm_state_create(&state_mode2, "MODE2", mode2_handler, &state_active);

    /* Create standby state under root */
    hsm_state_create(&state_standby, "STANDBY", standby_handler, &state_system);

    /* Initialize HSM with MODE1 as initial state */
    hsm_init(&my_hsm, "HierarchicalHSM", &state_mode1, NULL);

    printf("\n=== Testing State Hierarchy ===\n\n");

    /* Test 1: Button press in MODE1 */
    printf("--- Test 1: Button press in MODE1 ---\n");
    hsm_dispatch(&my_hsm, EVT_BUTTON_PRESS, NULL);

    /* Test 2: Mode change from MODE1 to MODE2 */
    printf("\n--- Test 2: Mode change (MODE1 -> MODE2) ---\n");
    hsm_dispatch(&my_hsm, EVT_MODE_CHANGE, NULL);

    /* Test 3: Button press in MODE2 */
    printf("\n--- Test 3: Button press in MODE2 ---\n");
    hsm_dispatch(&my_hsm, EVT_BUTTON_PRESS, NULL);

    /* Test 4: Common action (handled by root) */
    printf("\n--- Test 4: Common action (propagates to SYSTEM) ---\n");
    hsm_dispatch(&my_hsm, EVT_COMMON_ACTION, NULL);

    /* Test 5: Timeout (handled by ACTIVE parent) */
    printf("\n--- Test 5: Timeout (handled by ACTIVE parent) ---\n");
    hsm_dispatch(&my_hsm, EVT_TIMEOUT, NULL);

    /* Test 6: Wake from standby */
    printf("\n--- Test 6: Button press in STANDBY ---\n");
    hsm_dispatch(&my_hsm, EVT_BUTTON_PRESS, NULL);

    /* Test 7: Check state membership */
    printf("\n--- Test 7: Check state membership ---\n");
    if (hsm_is_in_state(&my_hsm, &state_mode1)) {
        printf("HSM is in MODE1 state\n");
    }
    if (hsm_is_in_state(&my_hsm, &state_active)) {
        printf("HSM is in ACTIVE parent state\n");
    }
    if (hsm_is_in_state(&my_hsm, &state_system)) {
        printf("HSM is in SYSTEM root state\n");
    }

    printf("\n=== Example Complete ===\n");
}
