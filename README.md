# HSM v1.0.1 - Hierarchical State Machine Library

🔥 **CRITICAL UPDATE**: Fixes race condition causing system resets

## Quick Install

### ESP-IDF (Recommended)
```bash
cd your_project/components
git clone https://github.com/phamnamhien/HSM.git
cd HSM
git checkout v1.0.1
```

### Or copy files directly
Copy these files to your project:
- `hsm.c` / `hsm.h` - Core library
- `hsm_config.h` - Configuration
- `examples/` - Platform examples

## What's Fixed in v1.0.1

✅ **Race condition eliminated** - No more random resets  
✅ **Automatic timer cleanup** - No manual deletion needed  
✅ **Thread-safe implementation** - Mutex protection added  
✅ **100% backward compatible** - No code changes required  

## File Structure

```
HSM_v1.0.1/
├── README.md                          ← This file
├── CHANGELOG.md                       ← Full version history
├── RELEASE_NOTES_v1.0.1.md          ← Detailed release notes
├── idf_component.yml                 ← ESP-IDF component manifest
├── hsm.c                             ← Core implementation
├── hsm.h                             ← Core header
├── hsm_config.h                      ← Configuration
├── Kconfig                           ← ESP-IDF menuconfig
└── examples/
    ├── timer_example_esp32.c         ← ESP32 with FreeRTOS (UPDATED)
    ├── timer_advanced_example.c      ← Advanced usage (UPDATED)
    ├── basic_example.c               ← Simple state machine
    ├── hierarchical_example.c        ← Nested states
    └── transition_param_example.c    ← Parameter passing
```

## Quick Start

```c
#include "hsm.h"

// 1. Define states
hsm_state_t state_idle, state_running;

// 2. Create state handlers
hsm_event_t idle_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            printf("Idle state\n");
            break;
        case EVT_START:
            hsm_transition(hsm, &state_running, NULL, NULL);
            break;
    }
    return HSM_EVENT_NONE;
}

// 3. Initialize HSM
hsm_t my_hsm;
hsm_state_create(&state_idle, "Idle", idle_handler, NULL);
hsm_init(&my_hsm, "MyHSM", &state_idle, NULL);

// 4. Dispatch events
hsm_dispatch(&my_hsm, EVT_START, NULL);
```

## v1.0.1 Changes

**Before:**
```c
case HSM_EVENT_EXIT:
    hsm_timer_delete(my_timer);  // Required
    break;
```

**After:**
```c
case HSM_EVENT_EXIT:
    // Timer auto-deleted by HSM!
    break;
```

## Support

- 📧 Email: phamnamhien@gmail.com
- 🐛 Issues: https://github.com/phamnamhien/HSM/issues
- 📖 Docs: See CHANGELOG.md and examples/

## License

MIT License
