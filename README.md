# HSM - Hierarchical State Machine Library

[![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)](https://github.com/phamnamhien/HSM)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20STM32%20%7C%20AVR-orange.svg)](README.md)

A lightweight and efficient Hierarchical State Machine (HSM) library for embedded systems written in C.

## 🔥 What's New in v2.0.0

**BREAKING CHANGE**: Timer support has been completely removed to keep the library minimal and focused on core state machine functionality.

### Key Changes
- ✅ **Pure state machine** - Focus on states, events, and transitions only
- ✅ **Smaller footprint** - Reduced memory usage (~20 bytes per HSM)
- ✅ **Zero dependencies** - No platform-specific code required
- ✅ **Simpler API** - Cleaner interface without timer complexity

**Migration from v1.x**: If you need timers, implement them in your application layer and dispatch HSM events from timer callbacks.

---

## Features

- **Hierarchical State Management**: Support for nested states with parent-child relationships
- **Event-driven Architecture**: Clean event handling with propagation up the state hierarchy
- **Safe Transitions**: Can safely call hsm_transition() inside ENTRY event handler with deferred execution
- **State History**: Optional history state feature to return to previous states
- **Memory Efficient**: Minimal RAM footprint (~20 bytes per HSM) suitable for resource-constrained systems
- **Configurable**: Adjustable maximum depth and optional features
- **Type-safe**: Strong typing with clear return values
- **Zero Dependencies**: Only requires standard C library (stdint.h)
- **Clean API**: Simple and intuitive interface with parameter passing and transition hooks
- **Platform Independent**: No platform-specific code required

## Use Cases

- Embedded system control logic
- Protocol state machines (UART, CAN, Modbus, etc.)
- User interface state management
- Robot behavior control
- IoT device state management
- Industrial automation controllers

## Key Concepts

### States
States represent different modes or conditions in your system. Each state can:
- Handle events
- Have a parent state (for hierarchy)
- Execute entry/exit actions
- Propagate unhandled events to parent

### Events
Events trigger state transitions or actions. The library provides standard events:
- `HSM_EVENT_ENTRY` - Called when entering a state (safe to call hsm_transition here)
- `HSM_EVENT_EXIT` - Called when exiting a state
- `HSM_EVENT_USER` - Starting point for custom events

### Transitions
Transitions move between states, automatically handling:
- Exit actions for current state chain
- Optional transition method hook (for cleanup, logging)
- Entry actions for new state chain
- Finding the Lowest Common Ancestor (LCA)
- Parameter passing to ENTRY/EXIT events

## Quick Start

### Basic Example

```c
#include "hsm.h"

/* Define states */
hsm_state_t state_idle, state_running, state_error;

/* State handler for idle state */
hsm_event_t
state_idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("Entering idle state\n");
            break;
            
        case HSM_EVENT_EXIT:
            printf("Exiting idle state\n");
            break;
            
        case EVT_START:
            hsm_transition(hsm, &state_running, NULL, NULL);
            return HSM_EVENT_NONE;
            
        default:
            return event; /* Propagate to parent */
    }
    return HSM_EVENT_NONE;
}

/* State handler for running state */
hsm_event_t
state_running_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("Entering running state\n");
            break;
            
        case HSM_EVENT_EXIT:
            printf("Exiting running state\n");
            break;
            
        case EVT_STOP:
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
            
        case EVT_ERROR:
            hsm_transition(hsm, &state_error, NULL, NULL);
            return HSM_EVENT_NONE;
            
        default:
            return event;
    }
    return HSM_EVENT_NONE;
}

int main(void) {
    hsm_t my_hsm;
    
    /* Create states */
    hsm_state_create(&state_idle, "Idle", state_idle_handler, NULL);
    hsm_state_create(&state_running, "Running", state_running_handler, NULL);
    hsm_state_create(&state_error, "Error", state_error_handler, NULL);
    
    /* Initialize HSM with idle as initial state */
    hsm_init(&my_hsm, "MyHSM", &state_idle);
    
    /* Dispatch events */
    hsm_dispatch(&my_hsm, EVT_START, NULL);
    hsm_dispatch(&my_hsm, EVT_STOP, NULL);
    
    return 0;
}
```

### Hierarchical Example

```c
/* Parent state */
hsm_state_t state_parent;

/* Child states */
hsm_state_t state_child1, state_child2;

hsm_event_t
parent_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case EVT_COMMON:
            printf("Parent handling common event\n");
            return HSM_EVENT_NONE;
            
        default:
            return event;
    }
}

hsm_event_t
child1_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case EVT_SWITCH:
            hsm_transition(hsm, &state_child2, NULL, NULL);
            return HSM_EVENT_NONE;
            
        default:
            return event; /* Propagate to parent */
    }
}

int main(void) {
    hsm_t hsm;
    
    /* Create parent state */
    hsm_state_create(&state_parent, "Parent", parent_handler, NULL);
    
    /* Create child states with parent */
    hsm_state_create(&state_child1, "Child1", child1_handler, &state_parent);
    hsm_state_create(&state_child2, "Child2", child2_handler, &state_parent);
    
    hsm_init(&hsm, "HierarchicalHSM", &state_child1);
    
    /* EVT_COMMON will be handled by parent if child doesn't handle it */
    hsm_dispatch(&hsm, EVT_COMMON, NULL);
    
    return 0;
}
```

### Using Timers (Application Layer)

If you need timer functionality, implement it in your application:

```c
/* ESP32 example */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

TimerHandle_t my_timer;
hsm_t* my_hsm_ptr;

void timer_callback(TimerHandle_t xTimer) {
    /* Dispatch event to HSM */
    hsm_dispatch(my_hsm_ptr, EVT_TIMEOUT, NULL);
}

hsm_event_t
active_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            /* Create and start timer */
            my_timer = xTimerCreate("timeout", pdMS_TO_TICKS(5000), 
                                   pdFALSE, NULL, timer_callback);
            xTimerStart(my_timer, 0);
            break;
            
        case HSM_EVENT_EXIT:
            /* Stop and delete timer */
            if (my_timer) {
                xTimerStop(my_timer, 0);
                xTimerDelete(my_timer, 0);
                my_timer = NULL;
            }
            break;
            
        case EVT_TIMEOUT:
            printf("Timeout!\n");
            hsm_transition(hsm, &state_idle, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}
```

## Configuration

Edit these macros in `hsm_config.h` or define them in your build system:

```c
/* Maximum state hierarchy depth */
#define HSM_CFG_MAX_DEPTH 8

/* Enable state history feature */
#define HSM_CFG_HISTORY 1
```

## API Reference

### State Management

#### `hsm_state_create()`
```c
hsm_result_t hsm_state_create(hsm_state_t* state, const char* name, 
                               hsm_state_fn_t handler, hsm_state_t* parent);
```
Initialize a state structure.

**Returns**: `HSM_RES_OK` on success

#### `hsm_init()`
```c
hsm_result_t hsm_init(hsm_t* hsm, const char* name, hsm_state_t* initial_state);
```
Initialize HSM instance.

**Returns**: `HSM_RES_OK` on success

### Event Handling

#### `hsm_dispatch()`
```c
hsm_result_t hsm_dispatch(hsm_t* hsm, hsm_event_t event, void* data);
```
Dispatch event to current state.

**Returns**: `HSM_RES_OK` on success

### State Transitions

#### `hsm_transition()`
```c
hsm_result_t hsm_transition(hsm_t* hsm, hsm_state_t* target, void* param,
                             void (*method)(hsm_t* hsm, void* param));
```
Transition to target state with optional parameter and hook.

**Returns**: `HSM_RES_OK` on success

#### `hsm_transition_history()` (if HSM_CFG_HISTORY enabled)
```c
hsm_result_t hsm_transition_history(hsm_t* hsm);
```
Transition to previous state.

### Query Functions

#### `hsm_get_current_state()`
```c
hsm_state_t* hsm_get_current_state(hsm_t* hsm);
```
Get current active state.

#### `hsm_is_in_state()`
```c
uint8_t hsm_is_in_state(hsm_t* hsm, hsm_state_t* state);
```
Check if HSM is in specific state or its parent.

## Return Codes

```c
typedef enum {
    HSM_RES_OK = 0x00,           /* Operation success */
    HSM_RES_ERROR,               /* Generic error */
    HSM_RES_INVALID_PARAM,       /* Invalid parameter */
    HSM_RES_MAX_DEPTH,           /* Maximum depth exceeded */
} hsm_result_t;
```

## Best Practices

1. **Safe Transitions**: You can safely call `hsm_transition()` inside `HSM_EVENT_ENTRY` handler
2. **Always handle standard events**: Entry and Exit
3. **Return HSM_EVENT_NONE** when event is handled
4. **Return event** to propagate to parent
5. **Keep state handlers simple**: Delegate complex logic to separate functions
6. **Use meaningful event numbers**: Define custom events starting from `HSM_EVENT_USER`
7. **Check return codes**: Always check return values from API functions
8. **Implement timers externally**: Use platform timers and dispatch events to HSM

## Memory Usage

- HSM instance: ~20 bytes (base)
- State structure: ~12 bytes
- Stack usage: Proportional to state depth (typically < 100 bytes)

## Platform Requirements

- C11 compiler
- `<stdint.h>` support
- No dynamic memory allocation
- No external dependencies

## Integration

### For ESP-IDF Projects

1. Add as component in `components/hsm/`
2. Use `idf_component_register()` in CMakeLists.txt
3. Configure via menuconfig: `Component config → HSM Configuration`

### For Other Platforms

1. Add `hsm.c` and `hsm.h` to your project
2. Include `hsm.h` in your source files
3. Configure options in `hsm_config.h` if needed
4. Compile and link with your project

## Examples

See `examples/` directory for complete examples:
- `basic_example.c` - Simple 3-state machine
- `hierarchical_example.c` - Nested state hierarchy
- `transition_param_example.c` - Parameter passing and hooks

## License

MIT License - See LICENSE file for full license text.

## Author

Pham Nam Hien

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- Functions are documented
- Changes are tested
- Commit messages are clear

## Version History

- **v2.0.0** (2025-12-29) - Major simplification
  - Removed all timer functionality
  - Cleaner, more focused API
  - Reduced memory footprint
  - Platform-independent implementation
  
- **v1.0.1** (2025-12-18) - Critical bug fix
  - Fixed race condition causing system resets
  - Automatic timer cleanup on state transitions
  
- **v1.0.0** (2025-12-17) - Initial release
  - Hierarchical state machine core
  - Built-in timer support

## Support

For issues, questions, or contributions, please visit the GitHub repository.
