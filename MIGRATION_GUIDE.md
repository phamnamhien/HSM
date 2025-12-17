# HSM Multiple Timer Migration Guide

## Summary of Changes

### Version 1.x → Version 2.0 (Multiple Timer Support)

**Major Changes:**
1. **Multiple timers per HSM** - Each HSM can now have up to `HSM_CFG_MAX_TIMERS` (default: 4) independent timers
2. **Manual timer management** - Timers must be explicitly created, started, stopped, and deleted
3. **Auto-stop on transition** - Timers automatically stop when state transitions occur (but are NOT deleted)
4. **Enhanced timer API** - Added restart, is_running, set_period, set_event functions

---

## Configuration Changes

### New Config Option in `hsm_config.h`:

```c
/**
 * \brief           Maximum number of timers per HSM instance
 * Range: 0-16
 * Default: 4
 */
#ifndef HSM_CFG_MAX_TIMERS
#define HSM_CFG_MAX_TIMERS 4
#endif
```

---

## API Changes

### OLD API (Single Timer):

```c
/* Start timer - overwrites existing timer */
hsm_timer_start(hsm, EVT_TIMEOUT, 1000, HSM_TIMER_PERIODIC);

/* Stop timer */
hsm_timer_stop(hsm);
```

### NEW API (Multiple Timers):

```c
/* 1. Create timers (in state ENTRY or globally) */
hsm_timer_t *timer1, *timer2;

hsm_timer_create(&timer1, hsm, EVT_BLINK, 500, HSM_TIMER_PERIODIC);
hsm_timer_create(&timer2, hsm, EVT_TIMEOUT, 5000, HSM_TIMER_ONE_SHOT);

/* 2. Start timers */
hsm_timer_start(timer1);
hsm_timer_start(timer2);

/* 3. Stop specific timer */
hsm_timer_stop(timer1);

/* 4. Delete timers (in state EXIT) */
hsm_timer_delete(timer1);
hsm_timer_delete(timer2);
```

---

## New Timer Functions

```c
/* Create timer (allocates from pool) */
hsm_result_t hsm_timer_create(hsm_timer_t** timer, hsm_t* hsm, hsm_event_t event,
                               uint32_t period_ms, hsm_timer_mode_t mode);

/* Start/Stop/Restart */
hsm_result_t hsm_timer_start(hsm_timer_t* timer);
hsm_result_t hsm_timer_stop(hsm_timer_t* timer);
hsm_result_t hsm_timer_restart(hsm_timer_t* timer);

/* Delete timer (releases back to pool) */
hsm_result_t hsm_timer_delete(hsm_timer_t* timer);

/* Configuration changes */
hsm_result_t hsm_timer_set_period(hsm_timer_t* timer, uint32_t period_ms);
hsm_result_t hsm_timer_set_event(hsm_timer_t* timer, hsm_event_t event);

/* Query functions */
uint8_t hsm_timer_is_running(hsm_timer_t* timer);
hsm_timer_state_t hsm_timer_get_state(hsm_timer_t* timer);

/* Bulk operations */
hsm_result_t hsm_timer_stop_all(hsm_t* hsm);
hsm_result_t hsm_timer_delete_all(hsm_t* hsm);
```

---

## Migration Steps

### Step 1: Update Configuration

Add to your `hsm_config.h`:
```c
#define HSM_CFG_MAX_TIMERS 4  /* Adjust based on your needs */
```

### Step 2: Declare Timer Variables

Old code:
```c
/* No timer variables needed */
```

New code:
```c
/* Declare timer pointers (globally or in struct) */
static hsm_timer_t *timer_led_blink;
static hsm_timer_t *timer_timeout;
```

### Step 3: Update State ENTRY Handler

Old code:
```c
case HSM_EVENT_ENTRY:
    /* This would overwrite any existing timer! */
    hsm_timer_start(hsm, EVT_TIMEOUT, 5000, HSM_TIMER_ONE_SHOT);
    break;
```

New code:
```c
case HSM_EVENT_ENTRY:
    /* Create and start first timer */
    hsm_timer_create(&timer_led_blink, hsm, EVT_LED_TICK, 
                     500, HSM_TIMER_PERIODIC);
    hsm_timer_start(timer_led_blink);
    
    /* Create and start second timer */
    hsm_timer_create(&timer_timeout, hsm, EVT_TIMEOUT,
                     5000, HSM_TIMER_ONE_SHOT);
    hsm_timer_start(timer_timeout);
    break;
```

### Step 4: Update State EXIT Handler

Old code:
```c
case HSM_EVENT_EXIT:
    /* Timer was auto-stopped and auto-deleted */
    break;
```

New code:
```c
case HSM_EVENT_EXIT:
    /* Timers are auto-stopped on transition, but should be deleted manually */
    hsm_timer_delete(timer_led_blink);
    hsm_timer_delete(timer_timeout);
    break;
```

### Step 5: Handle Timer Events

Old code:
```c
case EVT_TIMEOUT:
    /* Handle timeout */
    break;
```

New code (same):
```c
case EVT_LED_TICK:
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    break;
    
case EVT_TIMEOUT:
    /* Handle timeout */
    hsm_timer_delete(timer_timeout);  /* Clean up one-shot timer */
    break;
```

---

## Complete Example: Before & After

### BEFORE (Single Timer):

```c
static hsm_event_t
state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            /* Start timer - overwrites any existing timer! */
            hsm_timer_start(hsm, EVT_TIMEOUT, 5000, HSM_TIMER_ONE_SHOT);
            break;
            
        case EVT_TIMEOUT:
            /* Save settings */
            save_to_eeprom();
            hsm_transition(hsm, &state_run, NULL, NULL);
            return HSM_EVENT_NONE;
    }
    return event;
}
```

### AFTER (Multiple Timers):

```c
static hsm_timer_t *timer_blink, *timer_timeout;

static hsm_event_t
state_setting_handler(hsm_t* hsm, hsm_event_t event, void* data) {
    switch (event) {
        case HSM_EVENT_ENTRY:
            /* Create LED blink timer */
            hsm_timer_create(&timer_blink, hsm, EVT_LED_TICK,
                           500, HSM_TIMER_PERIODIC);
            hsm_timer_start(timer_blink);
            
            /* Create timeout timer */
            hsm_timer_create(&timer_timeout, hsm, EVT_TIMEOUT,
                           5000, HSM_TIMER_ONE_SHOT);
            hsm_timer_start(timer_timeout);
            break;
            
        case HSM_EVENT_EXIT:
            /* Cleanup timers */
            hsm_timer_delete(timer_blink);
            hsm_timer_delete(timer_timeout);
            break;
            
        case EVT_LED_TICK:
            HAL_GPIO_TogglePin(LED_RUN_GPIO_Port, LED_RUN_Pin);
            break;
            
        case EVT_TIMEOUT:
            /* Save settings */
            save_to_eeprom();
            hsm_transition(hsm, &state_run, NULL, NULL);
            return HSM_EVENT_NONE;
            
        case EVT_BAUD_CHANGED:
            /* Restart timeout timer */
            hsm_timer_restart(timer_timeout);
            break;
    }
    return event;
}
```

---

## Platform Timer Implementation Changes

### Old Implementation:
```c
/* Single global timer */
static hsm_timer_t hsm_timer;

void* stm32_timer_start(...) {
    /* Setup one timer */
    hsm_timer.htim = &htim3;
    /* ... */
    return &hsm_timer;
}
```

### New Implementation:
```c
/* Array of timers */
static stm32_timer_t hsm_timers[HSM_CFG_MAX_TIMERS];

void* stm32_timer_start(...) {
    /* Find free slot */
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (!hsm_timers[i].active) {
            hsm_timers[i].htim = &htim3;
            /* ... */
            return &hsm_timers[i];
        }
    }
    return NULL;  /* No free slot */
}

/* IRQ handler checks all timers */
void hsm_timer_irq_handler(void) {
    for (uint8_t i = 0; i < HSM_CFG_MAX_TIMERS; i++) {
        if (hsm_timers[i].active && hsm_timers[i].callback != NULL) {
            hsm_timers[i].callback(hsm_timers[i].arg);
            /* ... */
        }
    }
}
```

---

## Important Notes

### Auto-Stop vs Auto-Delete

**Auto-stop on transition (✅ enabled):**
- When `hsm_transition()` is called, ALL running timers are automatically stopped
- Timer configuration is preserved (period, event, mode)
- Can restart stopped timers with `hsm_timer_start()` or `hsm_timer_restart()`

**Auto-delete on transition (❌ disabled):**
- Timers are NOT automatically deleted
- You must manually call `hsm_timer_delete()` in state EXIT handler
- This prevents memory leaks in the timer pool

### Best Practices

1. **Create timers in ENTRY handler**
   ```c
   case HSM_EVENT_ENTRY:
       hsm_timer_create(&timer, hsm, EVT, period, mode);
       hsm_timer_start(timer);
       break;
   ```

2. **Delete timers in EXIT handler**
   ```c
   case HSM_EVENT_EXIT:
       hsm_timer_delete(timer);
       break;
   ```

3. **Check timer creation result**
   ```c
   if (hsm_timer_create(&timer, hsm, EVT, 1000, HSM_TIMER_PERIODIC) == HSM_RES_NO_TIMER) {
       /* No free timer slot! */
   }
   ```

4. **Use hsm_timer_restart() for retriggerable timers**
   ```c
   case EVT_BUTTON_PRESS:
       hsm_timer_restart(debounce_timer);  /* Restart debounce */
       break;
   ```

---

## Error Codes

New return codes:
```c
HSM_RES_NO_TIMER         /* No free timer slot in pool */
HSM_RES_TIMER_NOT_FOUND  /* Timer not found */
```

---

## Memory Usage

**Old version:**
- ~24-28 bytes per HSM instance (single timer)

**New version:**
- Base: ~24 bytes per HSM instance
- Timers: ~20 bytes × `HSM_CFG_MAX_TIMERS`
- **Total:** ~24 + (20 × 4) = **~104 bytes per HSM** (with 4 timers)

**Recommendation:** Set `HSM_CFG_MAX_TIMERS` to actual need (2-4 is common)

---

## Troubleshooting

### Problem: `HSM_RES_NO_TIMER` returned from `hsm_timer_create()`

**Solution:** Increase `HSM_CFG_MAX_TIMERS` or delete unused timers

### Problem: Timer doesn't fire

**Check:**
1. Did you call `hsm_timer_start()`?
2. Is platform timer callback calling `hsm_timer_callback()`?
3. Check `hsm_timer_is_running()` returns 1

### Problem: Timer fires after state transition

**Reason:** Forgot to delete timer in EXIT handler

**Solution:** Always call `hsm_timer_delete()` in EXIT

---

## See Examples

- `multiple_timer_stm32_example.c` - Complete STM32 example with 2 timers
- `timer_advanced_example.c` - Generic example showing all timer features

