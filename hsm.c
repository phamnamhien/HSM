/**
 * \file            hsm.c
 * \brief           Hierarchical State Machine implementation
 */

/*
 * Copyright (c) 2025 Pham Nam Hien
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of HSM library.
 *
 * Author:          Pham Nam Hien
 */
#include "hsm.h"
#include <stddef.h>


/**
 * \brief           Calculate state depth in hierarchy
 * \param[in]       state: Pointer to state
 * \return          Depth level, 0 for root
 */
static uint8_t
prv_get_state_depth(hsm_state_t* state) {
    uint8_t depth;

    depth = 0;
    while (state != NULL && state->parent != NULL) {
        depth++;
        state = state->parent;
    }
    return depth;
}

/**
 * \brief           Find lowest common ancestor of two states
 * \param[in]       state1: First state
 * \param[in]       state2: Second state
 * \return          Pointer to common ancestor state
 */
static hsm_state_t*
prv_find_lca(hsm_state_t* state1, hsm_state_t* state2) {
    hsm_state_t *s1, *s2;
    uint8_t depth1, depth2;

    s1 = state1;
    s2 = state2;
    depth1 = prv_get_state_depth(s1);
    depth2 = prv_get_state_depth(s2);

    /* Equalize depth */
    while (depth1 > depth2) {
        s1 = s1->parent;
        depth1--;
    }
    while (depth2 > depth1) {
        s2 = s2->parent;
        depth2--;
    }

    /* Find common parent */
    while (s1 != s2) {
        s1 = s1->parent;
        s2 = s2->parent;
    }

    return s1;
}

/**
 * \brief           Execute state handler
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       state: State to execute
 * \param[in]       event: Event to handle
 * \param[in]       data: Event data
 * \return          Event return value
 */
static hsm_event_t
prv_execute_state(hsm_t* hsm, hsm_state_t* state, hsm_event_t event, void* data) {
    if (state != NULL && state->handler != NULL) {
        return state->handler(hsm, event, data);
    }
    return HSM_EVENT_NONE;
}

/**
 * \brief           Initialize HSM state
 * \param[in]       state: Pointer to state structure
 * \param[in]       name: State name
 * \param[in]       handler: State handler function
 * \param[in]       parent: Parent state, `NULL` for root state
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_state_create(hsm_state_t* state, const char* name, hsm_state_fn_t handler,
                 hsm_state_t* parent) {
    uint8_t depth;

    if (state == NULL || handler == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    depth = prv_get_state_depth(parent) + 1;
    if (depth >= HSM_CFG_MAX_DEPTH) {
        return HSM_RES_MAX_DEPTH;
    }

    state->handler = handler;
    state->parent = parent;
    state->name = name;

    return HSM_RES_OK;
}

/**
 * \brief           Initialize HSM instance
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       name: HSM name
 * \param[in]       initial_state: Initial state
 * \param[in]       timer_if: Timer interface (can be NULL if timer not needed)
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_init(hsm_t* hsm, const char* name, hsm_state_t* initial_state,
         const hsm_timer_if_t* timer_if) {
    if (hsm == NULL || initial_state == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    hsm->name = name;
    hsm->current = initial_state;
    hsm->initial = initial_state;
    hsm->next = NULL;
    hsm->depth = prv_get_state_depth(initial_state);
    hsm->in_transition = 0;
    hsm->timer_handle = NULL;
    hsm->timer_event = HSM_EVENT_NONE;
    hsm->timer_if = timer_if;

#if HSM_CFG_HISTORY
    hsm->history = NULL;
#endif /* HSM_CFG_HISTORY */

    /* Execute entry for initial state hierarchy */
    hsm->in_transition = 1;
    for (hsm_state_t* state = initial_state; state != NULL; state = state->parent) {
        prv_execute_state(hsm, state, HSM_EVENT_ENTRY, NULL);
    }
    hsm->in_transition = 0;

    /* Check if deferred transition was requested in ENTRY */
    if (hsm->next != NULL) {
        hsm_state_t* next_state = hsm->next;
        hsm->next = NULL;
        return hsm_transition(hsm, next_state, NULL, NULL);
    }

    return HSM_RES_OK;
}

/**
 * \brief           Dispatch event to current state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       event: Event to dispatch
 * \param[in]       data: Event data
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_dispatch(hsm_t* hsm, hsm_event_t event, void* data) {
    hsm_state_t* state;
    hsm_event_t evt;

    if (hsm == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    state = hsm->current;
    evt = event;

    /* Propagate event up the hierarchy until handled */
    while (state != NULL && evt != HSM_EVENT_NONE) {
        evt = prv_execute_state(hsm, state, evt, data);
        if (evt != HSM_EVENT_NONE) {
            state = state->parent;
        }
    }

    return HSM_RES_OK;
}

/**
 * \brief           Transition to target state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       target: Target state
 * \param[in]       param: Optional parameter passed to ENTRY and EXIT events
 * \param[in]       method: Optional function hook called between EXIT and ENTRY events
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_transition(hsm_t* hsm, hsm_state_t* target, void* param, void (*method)(hsm_t* hsm, void* param)) {
    hsm_state_t* lca;
    hsm_state_t* exit_path[HSM_CFG_MAX_DEPTH];
    hsm_state_t* entry_path[HSM_CFG_MAX_DEPTH];
    uint8_t exit_count, entry_count, i;

    if (hsm == NULL || target == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    /* If already in transition, defer this transition */
    if (hsm->in_transition) {
        hsm->next = target;
        return HSM_RES_OK;
    }

#if HSM_CFG_HISTORY
    /* Save current state to history */
    hsm->history = hsm->current;
#endif /* HSM_CFG_HISTORY */

    /* Stop current state timer if any */
    if (hsm->timer_handle != NULL && hsm->timer_if != NULL && hsm->timer_if->stop != NULL) {
        hsm->timer_if->stop(hsm->timer_handle);
        hsm->timer_handle = NULL;
        hsm->timer_event = HSM_EVENT_NONE;
    }

    /* Find lowest common ancestor */
    lca = prv_find_lca(hsm->current, target);

    /* Build exit path from current to LCA */
    exit_count = 0;
    for (hsm_state_t* state = hsm->current; state != lca; state = state->parent) {
        exit_path[exit_count++] = state;
    }

    /* Build entry path from LCA to target */
    entry_count = 0;
    for (hsm_state_t* state = target; state != lca; state = state->parent) {
        entry_path[entry_count++] = state;
    }

    hsm->in_transition = 1;

    /* Execute exit actions with param */
    for (i = 0; i < exit_count; i++) {
        prv_execute_state(hsm, exit_path[i], HSM_EVENT_EXIT, param);
    }

    /* Call optional transition method hook */
    if (method != NULL) {
        method(hsm, param);
    }

    /* Execute entry actions in reverse order with param */
    for (i = entry_count; i > 0; i--) {
        prv_execute_state(hsm, entry_path[i - 1], HSM_EVENT_ENTRY, param);
    }

    /* Update current state */
    hsm->current = target;
    hsm->depth = prv_get_state_depth(target);

    hsm->in_transition = 0;

    /* Check if deferred transition was requested in ENTRY */
    if (hsm->next != NULL) {
        hsm_state_t* next_state = hsm->next;
        hsm->next = NULL;
        return hsm_transition(hsm, next_state, NULL, NULL);
    }

    return HSM_RES_OK;
}

/**
 * \brief           Get current state
 * \param[in]       hsm: Pointer to HSM instance
 * \return          Pointer to current state
 */
hsm_state_t*
hsm_get_current_state(hsm_t* hsm) {
    if (hsm == NULL) {
        return NULL;
    }
    return hsm->current;
}

/**
 * \brief           Check if HSM is in specific state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       state: State to check
 * \return          `1` if in state or parent state, `0` otherwise
 */
uint8_t
hsm_is_in_state(hsm_t* hsm, hsm_state_t* state) {
    hsm_state_t* current;

    if (hsm == NULL || state == NULL) {
        return 0;
    }

    current = hsm->current;
    while (current != NULL) {
        if (current == state) {
            return 1;
        }
        current = current->parent;
    }

    return 0;
}

#if HSM_CFG_HISTORY
/**
 * \brief           Transition to history state
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_transition_history(hsm_t* hsm) {
    if (hsm == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    if (hsm->history == NULL) {
        return hsm_transition(hsm, hsm->initial, NULL, NULL);
    }

    return hsm_transition(hsm, hsm->history, NULL, NULL);
}
#endif /* HSM_CFG_HISTORY */

/**
 * \brief           Timer callback wrapper
 * \param[in]       arg: HSM instance pointer
 */
static void
prv_timer_callback(void* arg) {
    hsm_t* hsm = (hsm_t*)arg;
    if (hsm != NULL && hsm->timer_event != HSM_EVENT_NONE) {
        hsm_dispatch(hsm, hsm->timer_event, NULL);
    }
}

/**
 * \brief           Start timer for current state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       event: Event to dispatch when timer expires
 * \param[in]       period_ms: Timer period in milliseconds (min 1ms)
 * \param[in]       mode: Timer mode (one-shot or periodic)
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_timer_start(hsm_t* hsm, hsm_event_t event, uint32_t period_ms, hsm_timer_mode_t mode) {
    if (hsm == NULL || hsm->timer_if == NULL || hsm->timer_if->start == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    if (period_ms < 1 || event == HSM_EVENT_NONE) {
        return HSM_RES_INVALID_PARAM;
    }

    /* Stop existing timer if any */
    if (hsm->timer_handle != NULL && hsm->timer_if->stop != NULL) {
        hsm->timer_if->stop(hsm->timer_handle);
        hsm->timer_handle = NULL;
    }

    /* Save timer event */
    hsm->timer_event = event;

    /* Start new timer */
    hsm->timer_handle =
        hsm->timer_if->start(prv_timer_callback, hsm, period_ms, (mode == HSM_TIMER_PERIODIC) ? 1 : 0);

    if (hsm->timer_handle == NULL) {
        hsm->timer_event = HSM_EVENT_NONE;
        return HSM_RES_ERROR;
    }

    return HSM_RES_OK;
}

/**
 * \brief           Stop timer for current state
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t
hsm_timer_stop(hsm_t* hsm) {
    if (hsm == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    if (hsm->timer_handle != NULL && hsm->timer_if != NULL && hsm->timer_if->stop != NULL) {
        hsm->timer_if->stop(hsm->timer_handle);
        hsm->timer_handle = NULL;
        hsm->timer_event = HSM_EVENT_NONE;
    }

    return HSM_RES_OK;
}
