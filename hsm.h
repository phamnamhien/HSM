/**
 * \file            hsm.h
 * \brief           Hierarchical State Machine library
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
 * Version:         2.0.0
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
} hsm_result_t;

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
 * \brief           HSM instance structure
 */
typedef struct hsm {
    hsm_state_t* current;                     /*!< Current state */
    hsm_state_t* initial;                     /*!< Initial state */
    hsm_state_t* next;                        /*!< Deferred transition target */
    const char* name;                         /*!< HSM name for debugging */
    uint8_t depth;                            /*!< Current state depth */
    uint8_t in_transition;                    /*!< Transition in progress flag */
    
#if HSM_CFG_HISTORY
    hsm_state_t* history;                     /*!< Previous state for history */
#endif /* HSM_CFG_HISTORY */
} hsm_t;

/**
 * \defgroup        HSM_API API Functions
 * \brief           HSM API functions
 * \{
 */

/* Initialization functions */
hsm_result_t hsm_init(hsm_t* hsm, const char* name, hsm_state_t* initial_state);
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

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HSM_HDR_H */
