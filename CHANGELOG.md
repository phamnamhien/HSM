# Changelog

All notable changes to this project will be documented in this file.

## [2.0.0] - 2025-12-29

### Breaking Changes
- **REMOVED**: All timer functionality has been removed from the library
- **CHANGED**: `hsm_init()` no longer requires `timer_if` parameter
- **REMOVED**: All timer-related API functions and types

### Added
- Pure state machine implementation focused on core functionality
- Cleaner, simpler API without timer complexity
- Platform-independent implementation (no platform-specific code)

### Changed
- Reduced memory footprint (~20 bytes per HSM instance)
- Simplified configuration (removed timer settings)
- Updated all examples to reflect timer removal

### Migration Guide
If you need timer functionality:
1. Implement timers in your application layer using platform APIs
2. Dispatch HSM events from timer callbacks
3. Manage timer lifecycle in state ENTRY/EXIT handlers

Example:
```c
// Old v1.x code:
hsm_timer_create(&timer, hsm, EVT_TIMEOUT, 1000, HSM_TIMER_ONE_SHOT);
hsm_timer_start(timer);

// New v2.0 approach:
my_timer = platform_timer_create(1000, timer_callback);
platform_timer_start(my_timer);

void timer_callback(void) {
    hsm_dispatch(&my_hsm, EVT_TIMEOUT, NULL);
}
```

## [1.0.1] - 2025-12-18

### Fixed
- **CRITICAL**: Fixed race condition that caused random system resets when transitioning between states with active timers
- Timer callbacks could execute after timers were deleted, accessing freed memory

### Changed
- Timers are now automatically deleted during state transitions (call `hsm_timer_delete_all()`)
- Added thread-safe timer implementation for ESP32 using mutex protection
- Manual timer cleanup in EXIT handlers is now optional

### Security
- Eliminated use-after-free vulnerability in timer system

## [1.0.0] - 2025-12-17

### Added
- Initial release
- Hierarchical state machine core
- Event dispatching with safe deferred transitions
- State transitions with parameter passing and hooks
- Built-in timer support with custom events
- One-shot and periodic timer modes
- History state support
- Configuration options for max depth, history, and timers
- ESP-IDF component integration
- Kconfig support
- Examples: basic, hierarchical, timer, transition parameters

### Features
- Memory efficient (~24-28 bytes per HSM + ~20 bytes per timer)
- Zero dependencies (except optional timer interface)
- Type-safe API
- Platform abstraction for timers (ESP32, STM32, AVR examples)

[2.0.0]: https://github.com/phamnamhien/HSM/releases/tag/v2.0.0
[1.0.1]: https://github.com/phamnamhien/HSM/releases/tag/v1.0.1
[1.0.0]: https://github.com/phamnamhien/HSM/releases/tag/v1.0.0
