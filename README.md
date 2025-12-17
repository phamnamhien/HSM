# HSM - Hierarchical State Machine Library

A lightweight and efficient Hierarchical State Machine (HSM) library for embedded systems written in C.

## Features

- **Hierarchical State Management**: Support for nested states with parent-child relationships
- **Event-driven Architecture**: Clean event handling with propagation up the state hierarchy
- **State History**: Optional history state feature to return to previous states
- **Memory Efficient**: Minimal RAM footprint suitable for resource-constrained systems
- **Configurable**: Adjustable maximum depth and optional features
- **Type-safe**: Strong typing with clear return values
- **Zero Dependencies**: Only requires standard C library (stdint.h)
- **Clean API**: Simple and intuitive interface

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
- `HSM_EVENT_ENTRY` - Called when entering a state
- `HSM_EVENT_EXIT` - Called when exiting a state
- `HSM_EVENT_INIT` - Called after entering for initialization
- `HSM_EVENT_USER` - Starting point for custom events

### Transitions
Transitions move between states, automatically handling:
- Exit actions for current state chain
- Entry actions for new state chain
- Finding the Lowest Common Ancestor (LCA)
- Calling initialization

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
            /* Entry action */
            printf("Entering idle state\n");
            break;
            
        case HSM_EVENT_EXIT:
            /* Exit action */
            printf("Exiting idle state\n");
            break;
            
        case HSM_EVENT_USER + 1: /* START event */
            hsm_transition(hsm, &state_running);
            break;
            
        default:
            return event; /* Propagate to parent */
    }
    return HSM_EVENT_NONE; /* Event handled */
}

/* State handler for running state */
hsm_event_t
state_running_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("Entering running state\n");
            break;
            
        case HSM_EVENT_USER + 2: /* STOP event */
            hsm_transition(hsm, &state_idle);
            break;
            
        case HSM_EVENT_USER + 3: /* ERROR event */
            hsm_transition(hsm, &state_error);
            break;
            
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
    hsm_dispatch(&my_hsm, HSM_EVENT_USER + 1, NULL); /* START */
    hsm_dispatch(&my_hsm, HSM_EVENT_USER + 2, NULL); /* STOP */
    
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
        case HSM_EVENT_USER + 10: /* Common event handled by parent */
            printf("Parent handling common event\n");
            break;
            
        default:
            return event;
    }
    return HSM_EVENT_NONE;
}

hsm_event_t
child1_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_USER + 1:
            hsm_transition(hsm, &state_child2);
            break;
            
        default:
            return event; /* Propagate to parent */
    }
    return HSM_EVENT_NONE;
}

int main(void) {
    hsm_t hsm;
    
    /* Create parent state */
    hsm_state_create(&state_parent, "Parent", parent_handler, NULL);
    
    /* Create child states with parent */
    hsm_state_create(&state_child1, "Child1", child1_handler, &state_parent);
    hsm_state_create(&state_child2, "Child2", child2_handler, &state_parent);
    
    hsm_init(&hsm, "HierarchicalHSM", &state_child1);
    
    /* Event 10 will be handled by parent if child doesn't handle it */
    hsm_dispatch(&hsm, HSM_EVENT_USER + 10, NULL);
    
    return 0;
}
```

## Configuration

Edit these macros in `hsm.h` or define them in your build system:

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
- `state` - Pointer to state structure
- `name` - State name (for debugging)
- `handler` - State handler function
- `parent` - Parent state, or `NULL` for root state

**Returns**: `HSM_RES_OK` on success

#### `hsm_init()`
```c
hsm_result_t hsm_init(hsm_t* hsm, const char* name, hsm_state_t* initial_state,
                      const hsm_timer_if_t* timer_if);
```
Initialize HSM instance.
- `hsm` - Pointer to HSM instance
- `name` - HSM name (for debugging)
- `initial_state` - Initial state
- `timer_if` - Timer interface (can be NULL if timer not needed)

**Returns**: `HSM_RES_OK` on success

### Event Handling

#### `hsm_dispatch()`
```c
hsm_result_t hsm_dispatch(hsm_t* hsm, hsm_event_t event, void* data);
```
Dispatch event to current state.
- `hsm` - Pointer to HSM instance
- `event` - Event to dispatch
- `data` - Event data pointer

**Returns**: `HSM_RES_OK` on success

### State Transitions

#### `hsm_transition()`
```c
hsm_result_t hsm_transition(hsm_t* hsm, hsm_state_t* target, void* param,
                             void (*method)(hsm_t* hsm, void* param));
```
Transition to target state.
- `hsm` - Pointer to HSM instance
- `target` - Target state
- `param` - Optional parameter passed to ENTRY and EXIT events (can be NULL)
- `method` - Optional hook function called between EXIT and ENTRY events (can be NULL)

**Returns**: `HSM_RES_OK` on success

**Example**:
```c
// Simple transition
hsm_transition(hsm, &state_idle, NULL, NULL);

// With parameter
transition_data_t data = {.error_code = 1001};
hsm_transition(hsm, &state_error, &data, NULL);

// With cleanup hook
hsm_transition(hsm, &state_connected, &data, cleanup_hook);
```

#### `hsm_transition_history()` (if HSM_CFG_HISTORY enabled)
```c
hsm_result_t hsm_transition_history(hsm_t* hsm);
```
Transition to previous state.

### Timer Functions

#### `hsm_timer_start()`
```c
hsm_result_t hsm_timer_start(hsm_t* hsm, hsm_event_t event, uint32_t period_ms,
                              hsm_timer_mode_t mode);
```
Start timer for current state.
- `hsm` - Pointer to HSM instance
- `event` - Custom event to dispatch when timer expires
- `period_ms` - Timer period in milliseconds (minimum 1ms)
- `mode` - Timer mode: `HSM_TIMER_ONE_SHOT` or `HSM_TIMER_PERIODIC`

**Returns**: `HSM_RES_OK` on success

**Notes**:
- Timer automatically stops when transitioning to another state
- Only one timer active per HSM instance
- Calling `hsm_timer_start()` again will stop the previous timer

**Example**:
```c
// One-shot timer (fires once after 50ms)
hsm_timer_start(hsm, EVT_DEBOUNCE_DONE, 50, HSM_TIMER_ONE_SHOT);

// Periodic timer (fires every 1000ms)
hsm_timer_start(hsm, EVT_BLINK_TIMEOUT, 1000, HSM_TIMER_PERIODIC);
```

#### `hsm_timer_stop()`
```c
hsm_result_t hsm_timer_stop(hsm_t* hsm);
```
Manually stop the timer.

### Query Functions

#### `hsm_get_current_state()`
```c
hsm_state_t* hsm_get_current_state(hsm_t* hsm);
```
Get current active state.

**Returns**: Pointer to current state

#### `hsm_is_in_state()`
```c
uint8_t hsm_is_in_state(hsm_t* hsm, hsm_state_t* state);
```
Check if HSM is in specific state or its parent.

**Returns**: `1` if in state, `0` otherwise

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

1. **Always handle standard events**: Entry, Exit, and Init
2. **Return HSM_EVENT_NONE** when event is handled
3. **Return event** to propagate to parent
4. **Keep state handlers simple**: Delegate complex logic to separate functions
5. **Use meaningful event numbers**: Define custom events starting from `HSM_EVENT_USER`
6. **Check return codes**: Always check return values from API functions

## Memory Usage

- HSM instance: ~12-16 bytes (depending on configuration)
- State structure: ~12 bytes
- Stack usage: Proportional to state depth (typically < 100 bytes)

## Platform Requirements

- C99 or later
- `<stdint.h>` support
- No dynamic memory allocation
- No external dependencies

## Integration

1. Add `hsm.c` and `hsm.h` to your project
2. Include `hsm.h` in your source files
3. Configure options in `hsm.h` if needed
4. Compile and link with your project

## License

MIT License - See source files for full license text.

## Author

Pham Nam Hien - Embedded Systems Engineer

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- Functions are documented
- Changes are tested
- Commit messages are clear

## Version History

- v1.0.0 - Initial release
  - Hierarchical state machine core
  - Event dispatching
  - State transitions
  - History state support
  - Configuration options

## Support

For issues, questions, or contributions, please contact the author or create an issue in the repository.
