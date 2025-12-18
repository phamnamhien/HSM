# Changelog

All notable changes to this project will be documented in this file.

## [1.0.1] - 2025-12-18

### 🐛 Critical Bug Fixes

#### Fixed Race Condition in Timer System
- **CRITICAL**: Fixed race condition that caused system resets when transitioning between states with active timers
- Changed `hsm_timer_stop_all()` to `hsm_timer_delete_all()` in `hsm_transition()` to prevent dangling timer callbacks
- Timer callbacks can no longer fire after state transitions, eliminating potential crashes

#### Platform Timer Adapter Improvements
- Added thread-safe timer implementation for ESP32 FreeRTOS
- Implemented per-timer mutex protection to prevent concurrent access
- Added safe deletion mechanism with deferred cleanup
- Timer callbacks now properly check state before execution

### 🔧 Technical Details

**Root Cause:**
When `hsm_transition()` was called while a timer was active, the following race condition could occur:
1. Timer callback fires
2. `hsm_transition()` calls `hsm_timer_stop_all()` → stops platform timer but keeps timer structure
3. EXIT handler tries to delete timer
4. Timer callback may still be executing → accesses freed memory → **CRASH/RESET**

**Solution:**
```c
// Before (v1.0.0)
hsm_timer_stop_all(hsm);  // Only stopped timers

// After (v1.0.1)
hsm_timer_delete_all(hsm);  // Completely deletes timers
```

### 📝 Breaking Changes

**None** - This is a bug fix release with no API changes

### 🚀 Migration Guide

If you have custom platform timer adapters, update them to include thread-safety:

**Before:**
```c
typedef struct {
    TimerHandle_t handle;
    void (*callback)(void*);
    void* arg;
} esp32_timer_t;
```

**After:**
```c
typedef struct {
    TimerHandle_t handle;
    void (*callback)(void*);
    void* arg;
    volatile timer_state_t state;
    SemaphoreHandle_t mutex;  // ← Add mutex protection
} esp32_timer_t;
```

### ✅ Updated Examples

- **timer_example_esp32.c**: Added thread-safe implementation with mutex protection
- **timer_advanced_example.c**: Updated to demonstrate safe timer usage patterns
- All examples now work reliably without random resets

### 🎯 Recommendations

**For application developers:**
- Update to v1.0.1 immediately if experiencing random resets
- No code changes required in your state handlers
- Timer deletion in EXIT handlers is now optional (HSM handles it automatically)

**For platform adapter developers:**
- Review the updated `timer_example_esp32.c` for thread-safe implementation
- Implement mutex protection in your timer callbacks
- Add deferred deletion with proper synchronization

### 📊 Memory Impact

- Per-timer overhead increased by ~8 bytes (for mutex handle)
- Total additional memory: ~8 × `HSM_CFG_MAX_TIMERS` bytes
- Example: With 4 timers = +32 bytes per HSM instance

### 🔍 Testing

Tested scenarios:
- ✅ Rapid state transitions with active timers
- ✅ Timer firing during state transition
- ✅ Multiple concurrent timers
- ✅ One-shot and periodic timers
- ✅ Stress test: 1000+ transitions with active timers - **NO CRASHES**

### 🙏 Acknowledgments

Special thanks to users who reported the issue and provided detailed crash logs.

---

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
Pham Nam Hien (phamnamhien)
