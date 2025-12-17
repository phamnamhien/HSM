/**
 * \file            hsm_config.h
 * \brief           HSM library configuration template
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
#ifndef HSM_CONFIG_HDR_H
#define HSM_CONFIG_HDR_H

/**
 * \brief           Enable ESP-IDF Kconfig integration
 * 
 * When enabled, configuration is read from sdkconfig.h
 * Otherwise, use manual configuration below
 */
#ifndef HSM_CFG_USE_KCONFIG
#define HSM_CFG_USE_KCONFIG 0
#endif

#if HSM_CFG_USE_KCONFIG
#include "sdkconfig.h"

#define HSM_CFG_MAX_DEPTH CONFIG_HSM_MAX_DEPTH
#define HSM_CFG_HISTORY CONFIG_HSM_HISTORY

#else
/**
 * \brief           Maximum state hierarchy depth
 * 
 * This defines how many levels of nested states are supported.
 * Each level uses approximately 4 bytes of stack during transition.
 * 
 * Range: 2-16
 * Default: 8
 */
#ifndef HSM_CFG_MAX_DEPTH
#define HSM_CFG_MAX_DEPTH 8
#endif

/**
 * \brief           Enable state history feature
 * 
 * When enabled, the HSM keeps track of the last active state,
 * allowing transition back to that state using hsm_transition_history().
 * 
 * Adds 4 bytes to HSM instance size.
 * 
 * Default: 1 (enabled)
 */
#ifndef HSM_CFG_HISTORY
#define HSM_CFG_HISTORY 1
#endif

/**
 * \brief           Maximum number of timers per HSM instance
 * 
 * Each timer consumes approximately 16-20 bytes of RAM.
 * Set to 0 to disable timer support completely.
 * 
 * Range: 0-16
 * Default: 4
 */
#ifndef HSM_CFG_MAX_TIMERS
#define HSM_CFG_MAX_TIMERS 4
#endif

#endif /* HSM_CFG_USE_KCONFIG */

#endif /* HSM_CONFIG_HDR_H */
