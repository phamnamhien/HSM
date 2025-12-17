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
    uint8_t depth = 0;
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
 * \return          Pointer to LCA state
 */
static hsm_state_t*
prv_find_lca(hsm_state_t* state1, hsm_state_t* state2) {
    uint8_t depth1, depth2;
    hsm_state_t *s1, *s2;

    if (state1 == NULL || state2 == NULL) {
        return NULL;
    }

    s1 = state1;
    s2 = state2;
    depth1 = prv_get_state_depth(s1);
    depth2 = prv_get_state_depth(s2);

    /* Bring both states to same level */
    while (depth1 > depth2) {
        s1 = s1->parent;
        depth1--;
    }
    while (depth2 > depth1) {
        s2 = s2->parent;
        depth2--;
    }

    /* Find common ancestor */
    while (s1 != s2 && s1 != NULL && s2 != NULL) {
        s1 = s1->parent;
        s2 = s2->parent;
    }

    return s1;
}

/**
 * \brief           Execute state handler
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       state: State to execute
 * \param[in]       event: Event to dispatch
 * \param[in]       data: Event data
 */
static void
prv_execute_state(hsm_t* hsm, hsm_state_t* state, hsm_event_t event, void* data) {
    if (state != NULL && state->handler != NULL) {
        state->handler(hsm, event, data);
    }
}

/**
 * \brief           Initialize HSM state
 * \param[in]       state: Pointer to state structure
 * \param[in]       name: State name
 * \param[in]       handler: State handler function
 * \param[in]       parent: Parent state (NULL for root)
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_state_create(hsm_state_t* state, const char* name, hsm_state_fn_t handler,
                 hsm_state_t* parent) {
    if (state == NULL || handler == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    state->name = name;
    state->handler = handler;
    state->parent = parent;

    return HSM_RES_OK;
}

/**
 * \brief           Initialize HSM instance
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       name: HSM name
 * \param[in]       initial_state: Initial state
 * \param[in]       timer_if: Timer interface (can be NULL)
 * \return          \ref HSM_RES_OK on success
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
    hsm->timer_if = timer_if;

#if HSM_CFG_HISTORY
    hsm->history = NULL;
#endif /* HSM_CFG_HISTORY */

#if HSM_CFG_MAX_TIMERS > 0
    /* Initialize timer pool */
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        hsm->timers[i].state = HSM_TIMER_STATE_IDLE;
        hsm->timers[i].handle = NULL;
        hsm->timers[i].hsm = hsm;
    }
#endif /* HSM_CFG_MAX_TIMERS */

    /* Enter initial state */
    hsm->in_transition = 1;
    prv_execute_state(hsm, initial_state, HSM_EVENT_ENTRY, NULL);
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
    return (hsm != NULL) ? hsm->current : NULL;
}

/**
 * \brief           Check if in specific state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       state: State to check
 * \return          1 if in state or parent, 0 otherwise
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

/**
 * \brief           Dispatch event to current state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       event: Event to dispatch
 * \param[in]       data: Event data
 * \return          \ref HSM_RES_OK on success
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

    /* Propagate event up the state hierarchy */
    while (state != NULL && evt != HSM_EVENT_NONE) {
        evt = state->handler(hsm, evt, data);
        state = state->parent;
    }

    return HSM_RES_OK;
}

/**
 * \brief           Transition to target state
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       target: Target state
 * \param[in]       param: Optional parameter passed to ENTRY and EXIT events
 * \param[in]       method: Optional hook function called between EXIT and ENTRY
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_transition(hsm_t* hsm, hsm_state_t* target, void* param,
               void (*method)(hsm_t* hsm, void* param)) {
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

#if HSM_CFG_MAX_TIMERS > 0
    /* Stop all running timers (but don't delete them) */
    hsm_timer_stop_all(hsm);
#endif /* HSM_CFG_MAX_TIMERS */

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

#if HSM_CFG_HISTORY
/**
 * \brief           Transition to history state
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success
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

/* ========================================================================== */
/*                            TIMER FUNCTIONS                                 */
/* ========================================================================== */

#if HSM_CFG_MAX_TIMERS > 0

/**
 * \brief           Create and configure a timer
 * \param[out]      timer: Pointer to timer pointer (will be set to allocated timer)
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       event: Event to dispatch when timer expires
 * \param[in]       period_ms: Timer period in milliseconds
 * \param[in]       mode: Timer mode
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_create(hsm_timer_t** timer, hsm_t* hsm, hsm_event_t event,
                 uint32_t period_ms, hsm_timer_mode_t mode) {
    if (timer == NULL || hsm == NULL || period_ms < 1) {
        return HSM_RES_INVALID_PARAM;
    }

    /* Find free timer slot */
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (hsm->timers[i].state == HSM_TIMER_STATE_IDLE) {
            hsm->timers[i].hsm = hsm;
            hsm->timers[i].event = event;
            hsm->timers[i].period_ms = period_ms;
            hsm->timers[i].mode = mode;
            hsm->timers[i].state = HSM_TIMER_STATE_STOPPED;
            hsm->timers[i].handle = NULL;
            
            *timer = &hsm->timers[i];
            return HSM_RES_OK;
        }
    }

    return HSM_RES_NO_TIMER; /* No free timer slot */
}

/**
 * \brief           Start a timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_start(hsm_timer_t* timer) {
    if (timer == NULL || timer->state == HSM_TIMER_STATE_IDLE) {
        return HSM_RES_INVALID_PARAM;
    }

    hsm_t* hsm = timer->hsm;
    if (hsm->timer_if == NULL || hsm->timer_if->start == NULL) {
        return HSM_RES_ERROR;
    }

    /* Stop if already running */
    if (timer->state == HSM_TIMER_STATE_RUNNING && timer->handle != NULL) {
        hsm->timer_if->stop(timer->handle);
    }

    /* Start platform timer */
    uint8_t repeat = (timer->mode == HSM_TIMER_PERIODIC) ? 1 : 0;
    timer->handle = hsm->timer_if->start(
        (void (*)(void*))hsm_timer_callback,
        timer,
        timer->period_ms,
        repeat
    );

    if (timer->handle == NULL) {
        return HSM_RES_ERROR;
    }

    timer->state = HSM_TIMER_STATE_RUNNING;
    return HSM_RES_OK;
}

/**
 * \brief           Stop a timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_stop(hsm_timer_t* timer) {
    if (timer == NULL || timer->state != HSM_TIMER_STATE_RUNNING) {
        return HSM_RES_INVALID_PARAM;
    }

    hsm_t* hsm = timer->hsm;
    if (hsm->timer_if != NULL && hsm->timer_if->stop != NULL && timer->handle != NULL) {
        hsm->timer_if->stop(timer->handle);
        timer->handle = NULL;
    }

    timer->state = HSM_TIMER_STATE_STOPPED;
    return HSM_RES_OK;
}

/**
 * \brief           Restart a timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_restart(hsm_timer_t* timer) {
    if (timer == NULL || timer->state == HSM_TIMER_STATE_IDLE) {
        return HSM_RES_INVALID_PARAM;
    }

    /* Stop if running */
    if (timer->state == HSM_TIMER_STATE_RUNNING) {
        hsm_timer_stop(timer);
    }

    /* Start again */
    return hsm_timer_start(timer);
}

/**
 * \brief           Delete a timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_delete(hsm_timer_t* timer) {
    if (timer == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    /* Stop if running */
    if (timer->state == HSM_TIMER_STATE_RUNNING) {
        hsm_timer_stop(timer);
    }

    /* Mark as idle */
    timer->state = HSM_TIMER_STATE_IDLE;
    timer->handle = NULL;
    timer->hsm = NULL;

    return HSM_RES_OK;
}

/**
 * \brief           Set timer period
 * \param[in]       timer: Pointer to timer structure
 * \param[in]       period_ms: New period in milliseconds
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_set_period(hsm_timer_t* timer, uint32_t period_ms) {
    if (timer == NULL || timer->state == HSM_TIMER_STATE_IDLE || period_ms < 1) {
        return HSM_RES_INVALID_PARAM;
    }

    timer->period_ms = period_ms;

    /* Restart if currently running */
    if (timer->state == HSM_TIMER_STATE_RUNNING) {
        return hsm_timer_restart(timer);
    }

    return HSM_RES_OK;
}

/**
 * \brief           Set timer event
 * \param[in]       timer: Pointer to timer structure
 * \param[in]       event: New event
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_set_event(hsm_timer_t* timer, hsm_event_t event) {
    if (timer == NULL || timer->state == HSM_TIMER_STATE_IDLE) {
        return HSM_RES_INVALID_PARAM;
    }

    timer->event = event;
    return HSM_RES_OK;
}

/**
 * \brief           Check if timer is running
 * \param[in]       timer: Pointer to timer structure
 * \return          1 if running, 0 otherwise
 */
uint8_t
hsm_timer_is_running(hsm_timer_t* timer) {
    if (timer == NULL) {
        return 0;
    }
    return (timer->state == HSM_TIMER_STATE_RUNNING) ? 1 : 0;
}

/**
 * \brief           Get timer state
 * \param[in]       timer: Pointer to timer structure
 * \return          Timer state
 */
hsm_timer_state_t
hsm_timer_get_state(hsm_timer_t* timer) {
    if (timer == NULL) {
        return HSM_TIMER_STATE_IDLE;
    }
    return timer->state;
}

/**
 * \brief           Stop all timers
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_stop_all(hsm_t* hsm) {
    if (hsm == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (hsm->timers[i].state == HSM_TIMER_STATE_RUNNING) {
            hsm_timer_stop(&hsm->timers[i]);
        }
    }

    return HSM_RES_OK;
}

/**
 * \brief           Delete all timers
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success
 */
hsm_result_t
hsm_timer_delete_all(hsm_t* hsm) {
    if (hsm == NULL) {
        return HSM_RES_INVALID_PARAM;
    }

    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (hsm->timers[i].state != HSM_TIMER_STATE_IDLE) {
            hsm_timer_delete(&hsm->timers[i]);
        }
    }

    return HSM_RES_OK;
}

/**
 * \brief           Timer callback - called from platform timer
 * \param[in]       timer: Pointer to timer structure
 */
void
hsm_timer_callback(hsm_timer_t* timer) {
    if (timer == NULL || timer->state != HSM_TIMER_STATE_RUNNING) {
        return;
    }

    hsm_t* hsm = timer->hsm;
    if (hsm == NULL) {
        return;
    }

    /* For one-shot timers, stop after firing */
    if (timer->mode == HSM_TIMER_ONE_SHOT) {
        timer->state = HSM_TIMER_STATE_STOPPED;
        /* Handle is stopped by platform, just clear it */
        timer->handle = NULL;
    }

    /* Dispatch timer event */
    hsm_dispatch(hsm, timer->event, NULL);
}

#endif /* HSM_CFG_MAX_TIMERS */
