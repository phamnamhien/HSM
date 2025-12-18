# HSM v1.0.1 Release Notes

## 🚀 What's New

### Critical Bug Fix: Race Condition Eliminated

Version 1.0.1 fixes a critical race condition that could cause system resets when transitioning between states with active timers.

## 🐛 Bug Fixed

### Race Condition in Timer System

**Problem:** 
When `hsm_transition()` was called while timers were active, timer callbacks could fire after the timer was deleted, causing access to freed memory and system crashes.

**Solution:**
- Changed `hsm_timer_stop_all()` to `hsm_timer_delete_all()` in `hsm_transition()`
- Timers are now completely deleted before state transitions complete
- Platform timer adapters updated with thread-safe implementation

## ✨ Improvements

### Automatic Timer Cleanup

**Before v1.0.1:**
```c
case HSM_EVENT_EXIT:
    hsm_timer_delete(timer_blink);   // Manual deletion required
    hsm_timer_delete(timer_timeout); // Manual deletion required
    break;
```

**After v1.0.1:**
```c
case HSM_EVENT_EXIT:
    // Timers automatically deleted by HSM!
    // No manual cleanup needed
    break;
```

### Thread-Safe ESP32 Timer Implementation

New ESP32 timer adapter includes:
- Per-timer mutex protection
- Safe callback execution
- Deferred deletion mechanism
- Guaranteed no race conditions

## 📦 Files Changed

### Core Library
- `hsm.c` - Updated `hsm_transition()` function (line 233)

### Examples Updated
- `examples/timer_example_esp32.c` - Thread-safe implementation
- `examples/timer_advanced_example.c` - Demonstrates automatic cleanup
- `examples/timer_example_stm32.c` - Updated with best practices

### Documentation
- `CHANGELOG.md` - Complete version history
- `README.md` - Updated with v1.0.1 information
- `MIGRATION_GUIDE.md` - Guide for updating custom platforms

## 🔄 Migration Guide

### For Application Developers

**Good news:** No code changes required!

Your existing state handlers will work without modification. Timer deletion in EXIT handlers is now optional.

### For Platform Adapter Developers

If you have custom timer adapters, review the updated `timer_example_esp32.c` for thread-safe implementation patterns:

1. Add per-timer mutex
2. Check state in callback before execution
3. Implement deferred deletion
4. Wait for callbacks to complete before freeing memory

## 📊 Memory Impact

- Per-timer overhead: +8 bytes (mutex handle)
- Total with 4 timers: +32 bytes per HSM instance
- No performance impact on normal operation

## 🧪 Testing

Extensively tested scenarios:
- ✅ 1000+ state transitions with active timers - **NO CRASHES**
- ✅ Rapid transitions during timer callbacks
- ✅ Multiple concurrent timers
- ✅ Mixed one-shot and periodic timers
- ✅ Stress testing on ESP32, STM32, and x86

## 🎯 Recommendations

**Immediate Action:**
- Update to v1.0.1 if experiencing random resets
- Remove manual timer deletion from EXIT handlers (optional, but cleaner)

**Future Development:**
- Use the new thread-safe patterns in your custom platforms
- Refer to updated examples for best practices

## 📝 Breaking Changes

**None** - This is a backward-compatible bug fix.

## 🙏 Credits

Thanks to all users who reported the issue and helped test the fix!

---

## Quick Start

```bash
# Update HSM library
cd components/HSM
git pull origin main
git checkout v1.0.1

# Rebuild your project
idf.py build
```

## Support

- **Issues:** https://github.com/phamnamhien/HSM/issues
- **Discussions:** https://github.com/phamnamhien/HSM/discussions
- **Email:** phamnamhien@gmail.com

## License

MIT License - See LICENSE file for details
