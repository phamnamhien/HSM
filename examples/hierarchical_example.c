/**
 * \file            hierarchical_example.c
 * \brief           Hierarchical state machine example
 */

#include "hsm.h"
#include <stdio.h>

/* Events */
typedef enum {
    EVT_BUTTON_PRESS = HSM_EVENT_USER,
    EVT_TIMEOUT,
    EVT_MODE_CHANGE,
    EVT_COMMON_ACTION,
} app_events_t;

/* States */
static hsm_state_t state_system;
static hsm_state_t state_active;
static hsm_state_t state_mode1;
static hsm_state_t state_mode2;
static hsm_state_t state_standby;

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
            printf("[SYSTEM] Common action handled\n");
            return HSM_EVENT_NONE;
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
            printf("[ACTIVE] Entry\n");
            break;

        case HSM_EVENT_EXIT:
            printf("[ACTIVE] Exit\n");
            break;

        case EVT_TIMEOUT:
            printf("[ACTIVE] Timeout -> STANDBY\n");
            hsm_transition(hsm, &state_standby, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
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
            printf("[MODE1] Change -> MODE2\n");
            hsm_transition(hsm, &state_mode2, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[MODE1] Button pressed\n");
            return HSM_EVENT_NONE;
    }
    return event;
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
            printf("[MODE2] Change -> MODE1\n");
            hsm_transition(hsm, &state_mode1, NULL, NULL);
            return HSM_EVENT_NONE;

        case EVT_BUTTON_PRESS:
            printf("[MODE2] Button pressed\n");
            return HSM_EVENT_NONE;
    }
    return event;
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
            printf("[STANDBY] Wake -> MODE1\n");
            hsm_transition(hsm, &state_mode1, NULL, NULL);
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

    printf("=== HSM Hierarchical Example ===\n\n");

    /* Create hierarchy:
     * SYSTEM (root)
     *   ├── ACTIVE
     *   │   ├── MODE1
     *   │   └── MODE2
     *   └── STANDBY
     */

    hsm_state_create(&state_system, "SYSTEM", system_handler, NULL);
    hsm_state_create(&state_active, "ACTIVE", active_handler, &state_system);
    hsm_state_create(&state_mode1, "MODE1", mode1_handler, &state_active);
    hsm_state_create(&state_mode2, "MODE2", mode2_handler, &state_active);
    hsm_state_create(&state_standby, "STANDBY", standby_handler, &state_system);

    /* Init */
    hsm_init(&my_hsm, "HierarchicalHSM", &state_mode1);

    /* Test */
    printf("\n--- Test 1: Button in MODE1 ---\n");
    hsm_dispatch(&my_hsm, EVT_BUTTON_PRESS, NULL);

    printf("\n--- Test 2: MODE1 -> MODE2 ---\n");
    hsm_dispatch(&my_hsm, EVT_MODE_CHANGE, NULL);

    printf("\n--- Test 3: Button in MODE2 ---\n");
    hsm_dispatch(&my_hsm, EVT_BUTTON_PRESS, NULL);

    printf("\n--- Test 4: Common action (propagate to SYSTEM) ---\n");
    hsm_dispatch(&my_hsm, EVT_COMMON_ACTION, NULL);

    printf("\n--- Test 5: Timeout (handled by ACTIVE) ---\n");
    hsm_dispatch(&my_hsm, EVT_TIMEOUT, NULL);

    printf("\n--- Test 6: Wake from STANDBY ---\n");
    hsm_dispatch(&my_hsm, EVT_BUTTON_PRESS, NULL);

    printf("\n--- Test 7: State membership ---\n");
    if (hsm_is_in_state(&my_hsm, &state_mode1)) {
        printf("In MODE1\n");
    }
    if (hsm_is_in_state(&my_hsm, &state_active)) {
        printf("In ACTIVE\n");
    }
    if (hsm_is_in_state(&my_hsm, &state_system)) {
        printf("In SYSTEM\n");
    }

    printf("\n=== Complete ===\n");
}
