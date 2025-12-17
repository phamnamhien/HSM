## 💖 Support This Project

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Q5Q1JW4XS)
[![PayPal](https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/phamnamhien)


# HSM - Hierarchical State Machine Library

A lightweight and efficient Hierarchical State Machine (HSM) library for embedded systems written in C.

## Features

- **Hierarchical State Management**: Support for nested states with parent-child relationships
- **Event-driven Architecture**: Clean event handling with propagation up the state hierarchy
- **Safe Transitions**: Can safely call hsm_transition() inside ENTRY event handler with deferred execution
- **Built-in Timer Support**: Platform-agnostic timer interface with one-shot and periodic modes
- **State History**: Optional history state feature to return to previous states
- **Memory Efficient**: Minimal RAM footprint (~24-28 bytes per HSM) suitable for resource-constrained systems
- **Configurable**: Adjustable maximum depth and optional features
- **Type-safe**: Strong typing with clear return values
- **Zero Dependencies**: Only requires standard C library (stdint.h)
- **Clean API**: Simple and intuitive interface with parameter passing and transition hooks

## Use Cases

- Embedded system control logic with timing requirements
- Protocol state machines (UART, CAN, Modbus, etc.)
- User interface state management
- Robot behavior control with state timeouts
- IoT device state management with auto-transitions
- Industrial automation controllers

## Key Concepts

### States
States represent different modes or conditions in your system. Each state can:
- Handle events
- Have a parent state (for hierarchy)
- Execute entry/exit actions
- Propagate unhandled events to parent
- Start timers that trigger custom events

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
- Timer auto-stop when leaving state
- Parameter passing to ENTRY/EXIT events

### Timers
Built-in timer support with:
- Custom events dispatched on timeout
- One-shot or periodic modes
- Platform abstraction (ESP32, STM32, AVR, etc.)
- Automatic cleanup on state transitions

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
            /* Entry action - safe to call transition here! */
            printf("Entering idle state\n");
            break;
            
        case HSM_EVENT_EXIT:
            /* Exit action */
            printf("Exiting idle state\n");
            break;
            
        case EVT_START: /* Custom event */
            hsm_transition(hsm, &state_running, NULL, NULL);
            return HSM_EVENT_NONE;
            
        default:
            return event; /* Propagate to parent */
    }
    return HSM_EVENT_NONE; /* Event handled */
}

/* State handler for running state with timer */
hsm_event_t
state_running_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("Entering running state\n");
            /* Start periodic timer - fires EVT_TIMEOUT every 1000ms */
            hsm_timer_start(hsm, EVT_TIMEOUT, 1000, HSM_TIMER_PERIODIC);
            break;
            
        case HSM_EVENT_EXIT:
            printf("Exiting running state\n");
            /* Timer auto-stops when leaving state */
            break;
            
        case EVT_TIMEOUT:
            printf("Timer tick\n");
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
    
    /* Timer interface (platform-specific) */
    const hsm_timer_if_t timer_if = {
        .start = platform_timer_start,
        .stop = platform_timer_stop,
        .get_ms = platform_get_ms
    };
    
    /* Create states */
    hsm_state_create(&state_idle, "Idle", state_idle_handler, NULL);
    hsm_state_create(&state_running, "Running", state_running_handler, NULL);
    hsm_state_create(&state_error, "Error", state_error_handler, NULL);
    
    /* Initialize HSM with idle as initial state */
    hsm_init(&my_hsm, "MyHSM", &state_idle, &timer_if);
    
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
        case EVT_COMMON: /* Common event handled by parent */
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
    
    hsm_init(&hsm, "HierarchicalHSM", &state_child1, NULL);
    
    /* EVT_COMMON will be handled by parent if child doesn't handle it */
    hsm_dispatch(&hsm, EVT_COMMON, NULL);
    
    return 0;
}
```

### Transition with Parameters Example

```c
typedef struct {
    uint32_t error_code;
    const char* message;
} error_data_t;

void cleanup_hook(hsm_t* hsm, void* param) {
    printf("Cleanup between states\n");
    /* Log to database, sync state, free resources, etc. */
}

hsm_event_t
error_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            if (data != NULL) {
                error_data_t* err = (error_data_t*)data;
                printf("Error: %s (code: %u)\n", err->message, err->error_code);
            }
            break;
    }
    return event;
}

/* Transition with parameter and hook */
error_data_t err = {.error_code = 1001, .message = "Connection failed"};
hsm_transition(hsm, &state_error, &err, cleanup_hook);
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

## Timer Modes

```c
typedef enum {
    HSM_TIMER_ONE_SHOT = 0,      /* Timer fires once */
    HSM_TIMER_PERIODIC = 1,      /* Timer fires repeatedly */
} hsm_timer_mode_t;
```

## Best Practices

1. **Safe Transitions**: You can safely call `hsm_transition()` inside `HSM_EVENT_ENTRY` handler
2. **Always handle standard events**: Entry and Exit
3. **Return HSM_EVENT_NONE** when event is handled
4. **Return event** to propagate to parent
5. **Keep state handlers simple**: Delegate complex logic to separate functions
6. **Use meaningful event numbers**: Define custom events starting from `HSM_EVENT_USER`
7. **Check return codes**: Always check return values from API functions
8. **Timer cleanup**: Timers automatically stop on state transitions
9. **Use custom events for timers**: Define unique events for different timer purposes

## Memory Usage

- HSM instance: ~24-28 bytes (depending on configuration)
- State structure: ~12 bytes
- Stack usage: Proportional to state depth (typically < 100 bytes)

## Platform Requirements

- C11 compiler
- `<stdint.h>` support
- No dynamic memory allocation
- No external dependencies (timer interface optional)

## Integration

### For ESP-IDF Projects

1. Add as component in `components/hsm/`
2. Use `idf_component_register()` in CMakeLists.txt
3. Configure via menuconfig: `Component config → HSM Configuration`

### For Other Platforms

1. Add `hsm.c` and `hsm.h` to your project
2. Include `hsm.h` in your source files
3. Configure options in `hsm.h` if needed
4. Implement timer interface if using timers
5. Compile and link with your project

## Examples

See `examples/` directory for complete examples:
- `basic_example.c` - Simple 3-state machine
- `hierarchical_example.c` - Nested state hierarchy
- `timer_example_esp32.c` - Timer with ESP32 FreeRTOS
- `timer_example_stm32.c` - Timer with STM32 HAL
- `timer_advanced_example.c` - One-shot and periodic timers
- `transition_param_example.c` - Parameter passing and hooks

## License

MIT License - See source files for full license text.

## Author

Pham Nam Hien (phamnamhien)

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- Functions are documented
- Changes are tested
- Commit messages are clear

## Version History

- v1.0.0 - Initial release
  - Hierarchical state machine core
  - Event dispatching with safe deferred transitions
  - State transitions with parameter passing and hooks
  - Built-in timer support with custom events
  - History state support
  - Configuration options
  - ESP-IDF component integration

## Support

For issues, questions, or contributions, please contact the author.
