# Changelog

All notable changes to this project will be documented in this file.

## [1.0.0] - 2025-12-17

### Added
- Initial release of HSM library
- Hierarchical state machine implementation with nested states
- Event-driven architecture with parent-child state relationships
- Safe state transitions with deferred execution support
- Built-in timer support with platform abstraction layer
- Custom event support for timers (one-shot and periodic modes)
- Transition parameter passing to ENTRY/EXIT events
- Transition method hooks for cleanup between states
- Optional state history feature
- Zero dynamic allocation design
- ESP-IDF component integration with Kconfig support
- Comprehensive examples for multiple platforms

### Core Features
- `hsm_init()` - Initialize HSM with optional timer interface
- `hsm_state_create()` - Create hierarchical states
- `hsm_dispatch()` - Dispatch events to state machine
- `hsm_transition()` - Transition with parameter and method hook support
- `hsm_timer_start()` - Start timer with custom events
- `hsm_timer_stop()` - Stop active timer
- `hsm_get_current_state()` - Query current state
- `hsm_is_in_state()` - Check if in specific state or parent
- `hsm_transition_history()` - Transition to previous state (optional)

### Platform Support
- ESP32 (ESP-IDF with FreeRTOS timers)
- STM32 (HAL timers)
- AVR and other microcontrollers (custom timer interface)
- Linux/Windows (for testing)

### Examples
- `basic_example.c` - Simple 3-state machine
- `hierarchical_example.c` - Nested state hierarchy
- `timer_example_esp32.c` - Timer integration for ESP32
- `timer_example_stm32.c` - Timer integration for STM32
- `timer_advanced_example.c` - One-shot and periodic timer usage
- `transition_param_example.c` - Parameter passing and method hooks

### Memory Footprint
- HSM instance: ~24-28 bytes (depending on configuration)
- State structure: ~12 bytes per state
- No dynamic memory allocation
- Configurable maximum depth (default: 8 levels)

### Documentation
- Complete API reference
- Quick start guide
- Platform integration examples
- Best practices guide
- Detailed inline code documentation

## Author
Pham Nam Hien (phamnamhien) - Embedded Systems Engineer

## License
MIT License - Based on Howard Chan's HSM library (https://github.com/howard-chan/HSM)
