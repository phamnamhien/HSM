/**
 * \file            hsm.h
 * \brief           Hierarchical State Machine library with multiple timer support
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
 * Version:         1.0.1
 */
#ifndef HSM_HDR_H
#define HSM_HDR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Include configuration */
#include "hsm_config.h"

/**
 * \defgroup        HSM_TYPES Type definitions
 * \brief           HSM type definitions
 * \{
 */

/**
 * \brief           HSM event type
 */
typedef uint32_t hsm_event_t;

/**
 * \brief           HSM result enumeration
 */
typedef enum {
    HSM_RES_OK = 0x00,                        /*!< Operation successful */
    HSM_RES_ERROR,                            /*!< Generic error */
    HSM_RES_INVALID_PARAM,                    /*!< Invalid parameter */
    HSM_RES_MAX_DEPTH,                        /*!< Maximum depth exceeded */
    HSM_RES_NO_TIMER,                         /*!< No timer available */
    HSM_RES_TIMER_NOT_FOUND,                  /*!< Timer not found */
} hsm_result_t;

/**
 * \brief           Timer mode enumeration
 */
typedef enum {
    HSM_TIMER_ONE_SHOT = 0,                   /*!< Timer fires once */
    HSM_TIMER_PERIODIC = 1,                   /*!< Timer fires repeatedly */
} hsm_timer_mode_t;

/**
 * \brief           Timer state enumeration
 */
typedef enum {
    HSM_TIMER_STATE_IDLE = 0,                 /*!< Timer not created */
    HSM_TIMER_STATE_STOPPED = 1,              /*!< Timer created but stopped */
    HSM_TIMER_STATE_RUNNING = 2,              /*!< Timer running */
} hsm_timer_state_t;

/**
 * \}
 */

/**
 * \defgroup        HSM_EVENTS Standard events
 * \brief           HSM standard event definitions
 * \{
 */

#define HSM_EVENT_NONE 0x00                   /*!< No event */
#define HSM_EVENT_ENTRY 0x01                  /*!< State entry event */
#define HSM_EVENT_EXIT 0x02                   /*!< State exit event */
#define HSM_EVENT_USER 0x10                   /*!< User events start from here */

/**
 * \}
 */

/**
 * \brief           Forward declarations
 */
struct hsm;
struct hsm_state;
struct hsm_timer;

/**
 * \brief           Timer callback function for platform-specific timer
 * \param[in]       callback: Function to call when timer expires
 * \param[in]       arg: Argument to pass to callback
 * \param[in]       period_ms: Timer period in milliseconds
 * \param[in]       repeat: 1 for repeating timer, 0 for one-shot
 * \return          Timer handle (platform specific), or NULL on error
 */
typedef void* (*hsm_timer_start_fn_t)(void (*callback)(void*), void* arg, uint32_t period_ms,
                                       uint8_t repeat);

/**
 * \brief           Stop timer callback function
 * \param[in]       timer_handle: Timer handle returned from start function
 */
typedef void (*hsm_timer_stop_fn_t)(void* timer_handle);

/**
 * \brief           Get current time in milliseconds
 * \return          Current time in milliseconds
 */
typedef uint32_t (*hsm_timer_get_ms_fn_t)(void);

/**
 * \brief           Timer interface structure
 */
typedef struct {
    hsm_timer_start_fn_t start;              /*!< Start timer function */
    hsm_timer_stop_fn_t stop;                /*!< Stop timer function */
    hsm_timer_get_ms_fn_t get_ms;            /*!< Get current time function */
} hsm_timer_if_t;

/**
 * \brief           State handler function prototype
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       event: Event to handle
 * \param[in]       data: Event data pointer
 * \return          Event to propagate to parent, or `HSM_EVENT_NONE` if handled
 */
typedef hsm_event_t (*hsm_state_fn_t)(struct hsm* hsm, hsm_event_t event, void* data);

/**
 * \brief           HSM state structure
 */
typedef struct hsm_state {
    hsm_state_fn_t handler;                   /*!< State handler function */
    struct hsm_state* parent;                 /*!< Parent state pointer */
    const char* name;                         /*!< State name for debugging */
} hsm_state_t;

/**
 * \brief           HSM timer structure
 */
typedef struct hsm_timer {
    void* handle;                             /*!< Platform timer handle */
    hsm_event_t event;                        /*!< Event to dispatch on timeout */
    uint32_t period_ms;                       /*!< Timer period in milliseconds */
    hsm_timer_mode_t mode;                    /*!< Timer mode (one-shot/periodic) */
    hsm_timer_state_t state;                  /*!< Timer state */
    struct hsm* hsm;                          /*!< Parent HSM instance */
} hsm_timer_t;

/**
 * \brief           HSM instance structure
 */
typedef struct hsm {
    hsm_state_t* current;                     /*!< Current state */
    hsm_state_t* initial;                     /*!< Initial state */
    hsm_state_t* next;                        /*!< Deferred transition target */
    const char* name;                         /*!< HSM name for debugging */
    uint8_t depth;                            /*!< Current state depth */
    uint8_t in_transition;                    /*!< Transition in progress flag */
    const hsm_timer_if_t* timer_if;           /*!< Timer interface */
    
#if HSM_CFG_HISTORY
    hsm_state_t* history;                     /*!< Previous state for history */
#endif /* HSM_CFG_HISTORY */

#if HSM_CFG_MAX_TIMERS > 0
    hsm_timer_t timers[HSM_CFG_MAX_TIMERS];   /*!< Timer pool */
#endif /* HSM_CFG_MAX_TIMERS */
} hsm_t;

/**
 * \defgroup        HSM_API API Functions
 * \brief           HSM API functions
 * \{
 */

/* Initialization functions */
hsm_result_t hsm_init(hsm_t* hsm, const char* name, hsm_state_t* initial_state,
                      const hsm_timer_if_t* timer_if);
hsm_result_t hsm_state_create(hsm_state_t* state, const char* name, hsm_state_fn_t handler,
                               hsm_state_t* parent);

/* Event handling */
hsm_result_t hsm_dispatch(hsm_t* hsm, hsm_event_t event, void* data);
hsm_result_t hsm_transition(hsm_t* hsm, hsm_state_t* target, void* param,
                             void (*method)(hsm_t* hsm, void* param));

/* Query functions */
hsm_state_t* hsm_get_current_state(hsm_t* hsm);
uint8_t hsm_is_in_state(hsm_t* hsm, hsm_state_t* state);

#if HSM_CFG_HISTORY
hsm_result_t hsm_transition_history(hsm_t* hsm);
#endif /* HSM_CFG_HISTORY */

/* Timer functions */
#if HSM_CFG_MAX_TIMERS > 0

/**
 * \brief           Create and configure a timer
 * \param[out]      timer: Pointer to timer structure
 * \param[in]       hsm: Pointer to HSM instance
 * \param[in]       event: Event to dispatch when timer expires
 * \param[in]       period_ms: Timer period in milliseconds (minimum 1ms)
 * \param[in]       mode: Timer mode (HSM_TIMER_ONE_SHOT or HSM_TIMER_PERIODIC)
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_create(hsm_timer_t** timer, hsm_t* hsm, hsm_event_t event,
                               uint32_t period_ms, hsm_timer_mode_t mode);
                               
/**
 * \brief           Start a configured timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_start(hsm_timer_t* timer);

/**
 * \brief           Stop a running timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_stop(hsm_timer_t* timer);

/**
 * \brief           Restart a timer (stop then start with same settings)
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_restart(hsm_timer_t* timer);

/**
 * \brief           Delete a timer
 * \param[in]       timer: Pointer to timer structure
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_delete(hsm_timer_t* timer);

/**
 * \brief           Change timer period
 * \param[in]       timer: Pointer to timer structure
 * \param[in]       period_ms: New period in milliseconds
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_set_period(hsm_timer_t* timer, uint32_t period_ms);

/**
 * \brief           Change timer event
 * \param[in]       timer: Pointer to timer structure
 * \param[in]       event: New event to dispatch
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_set_event(hsm_timer_t* timer, hsm_event_t event);

/**
 * \brief           Check if timer is running
 * \param[in]       timer: Pointer to timer structure
 * \return          1 if running, 0 otherwise
 */
uint8_t hsm_timer_is_running(hsm_timer_t* timer);

/**
 * \brief           Get timer state
 * \param[in]       timer: Pointer to timer structure
 * \return          Timer state (IDLE, STOPPED, RUNNING)
 */
hsm_timer_state_t hsm_timer_get_state(hsm_timer_t* timer);

/**
 * \brief           Stop all timers for this HSM instance
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_stop_all(hsm_t* hsm);

/**
 * \brief           Delete all timers for this HSM instance
 * \param[in]       hsm: Pointer to HSM instance
 * \return          \ref HSM_RES_OK on success, member of \ref hsm_result_t otherwise
 */
hsm_result_t hsm_timer_delete_all(hsm_t* hsm);

/**
 * \brief           Timer timeout callback - must be called from platform timer callback
 * \param[in]       timer: Pointer to timer structure
 */
void hsm_timer_callback(hsm_timer_t* timer);

#endif /* HSM_CFG_MAX_TIMERS */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HSM_HDR_H */
