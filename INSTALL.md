# HSM v1.0.1 Installation Guide

## Method 1: ESP-IDF Component (Recommended for ESP32)

```bash
cd your_project/components
unzip HSM_v1.0.1.zip
mv HSM_v1.0.1 HSM
cd ..
idf.py build
```

## Method 2: Git Submodule

```bash
cd your_project/components
git submodule add https://github.com/phamnamhien/HSM.git
cd HSM
git checkout v1.0.1
```

## Method 3: Manual Copy (Other Platforms)

For STM32, AVR, or other platforms:

1. Extract ZIP file
2. Copy to your project:
   - `hsm.c` → src/
   - `hsm.h` → include/
   - `hsm_config.h` → include/
3. Add to your build system
4. Implement timer interface (see examples/)

## Verify Installation

Create test file:

```c
#include "hsm.h"
#include <stdio.h>

hsm_state_t state_test;

hsm_event_t test_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    if (event == HSM_EVENT_ENTRY) {
        printf("HSM v1.0.1 working!\n");
    }
    return HSM_EVENT_NONE;
}

int main(void) {
    hsm_t hsm;
    hsm_state_create(&state_test, "Test", test_handler, NULL);
    hsm_init(&hsm, "TestHSM", &state_test, NULL);
    return 0;
}
```

Build and run:
```bash
idf.py build flash monitor  # ESP32
# Or your platform's build command
```

Expected output:
```
HSM v1.0.1 working!
```

## Configuration

Edit `hsm_config.h`:

```c
#define HSM_CFG_MAX_DEPTH 8      // Max nested levels
#define HSM_CFG_HISTORY 1        // Enable history
#define HSM_CFG_MAX_TIMERS 4     // Timers per HSM
```

Or use menuconfig (ESP-IDF):
```bash
idf.py menuconfig
# → Component config → HSM Configuration
```

## Next Steps

1. Read `CHANGELOG.md` for v1.0.1 changes
2. Check `examples/` for usage patterns
3. See `RELEASE_NOTES_v1.0.1.md` for migration guide

## Troubleshooting

**"hsm.h not found"**
→ Add include path to build system

**Link errors**
→ Add hsm.c to sources list

**Random resets (v1.0.0)**
→ Update to v1.0.1 immediately!

## Support

- Email: phamnamhien@gmail.com
- Issues: https://github.com/phamnamhien/HSM/issues
