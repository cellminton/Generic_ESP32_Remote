# Telegram Watchdog Fix (v2)# Telegram Watchdog Fix

## Problem## Problem

The ESP32 was experiencing watchdog timer resets when sending Telegram
notifications. The error occurred because:The ESP32 was experiencing watchdog
timer resets when sending Telegram

notifications. The error occurred because:

1. Telegram's HTTPS connection to send messages takes several seconds (5-10
   seconds)

2. The `bot->sendMessage()` and `bot->getUpdates()` calls are **completely
   blocking**1. Telegram's HTTPS connection to send messages takes several
   seconds (5-8

3. No code runs during the HTTPS transaction - you cannot feed the watchdog
   mid-operation seconds)

4. The watchdog timer expects to be fed every 5 seconds
   (TASK_WATCHDOG_TIMEOUT_SEC)2. The initial notification was being sent
   synchronously in `setup()` / early in

5. After 5 seconds without feeding, the watchdog triggers and resets the ESP32
   `loop()`

6. During this long blocking operation, the watchdog timer wasn't being fed

## Error Log4. After 5 seconds (TASK_WATCHDOG_TIMEOUT_SEC), the watchdog triggered and reset

````the ESP32

E (33885) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:

E (33885) task_wdt:  - loopTask (CPU 1)## Error Log

E (33885) task_wdt: Tasks currently running:

E (33885) task_wdt: CPU 0: IDLE0```

E (33885) task_wdt: CPU 1: IDLE1E (29899) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:

E (33885) task_wdt: Aborting.E (29899) task_wdt:  - loopTask (CPU 1)

```E (29899) task_wdt: Tasks currently running:

E (29899) task_wdt: CPU 0: IDLE0

## SolutionE (29899) task_wdt: CPU 1: loopTask

E (29899) task_wdt: Aborting.

Implemented a **watchdog suspension mechanism** that temporarily disables watchdog monitoring during long blocking operations.```



### Key Insight## Solution

Since the HTTPS calls are blocking and we cannot interrupt them to feed the watchdog, we must temporarily remove the task from watchdog monitoring entirely.

Implemented a multi-layered fix:

### 1. New WatchdogManager Methods

### 1. Deferred Notification System

Added two methods to temporarily disable watchdog monitoring:

- Added `queueIPNotification()` method that queues notifications instead of

**`suspend()`** - Removes the current task from watchdog monitoring  sending immediately

```cpp- Notifications are sent in the `loop()` method when safe

void WatchdogManager::suspend()- Prevents blocking during critical startup phase

{

    esp_task_wdt_delete(NULL);  // Remove task from WDT### 2. Watchdog Integration

}

```- TelegramNotifier now accepts a WatchdogManager pointer in `begin()`

- Watchdog is fed before and after long operations (sending messages)

**`resume()`** - Re-adds the current task and resets the timer- Watchdog is fed during message checking loops

```cpp

void WatchdogManager::resume()### 3. Code Changes

{

    esp_task_wdt_add(NULL);     // Add task back to WDT#### TelegramNotifier.h

    esp_task_wdt_reset();       // Reset the timer

}- Added `WatchdogManager` forward declaration

```- Added `begin(WatchdogManager *wdt)` parameter

- Added `queueIPNotification()` for non-blocking notification

### 2. Integration with TelegramNotifier- Added private members for deferred notification support



All blocking Telegram operations now follow this pattern:#### TelegramNotifier.cpp



```cpp- Modified constructor to initialize new member variables

// Suspend watchdog before blocking operation- `begin()` now accepts and stores watchdog reference

if (watchdog != nullptr)- `loop()` feeds watchdog and handles pending notifications

{- `sendIPAddress()` feeds watchdog before/after sending

    watchdog->suspend();- New `sendPendingNotification()` sends queued notifications

}

#### main.cpp

// Perform blocking operation (can take 5-10 seconds)

bool success = bot->sendMessage(TELEGRAM_CHAT_ID, message, "");- Changed `telegramNotifier->begin()` to

  `telegramNotifier->begin(&watchdogManager)`

// Resume watchdog after operation- Changed `sendIPAddress()` calls to `queueIPNotification()`

if (watchdog != nullptr)- Notifications now sent asynchronously in the main loop

{

    watchdog->resume();## How It Works Now

}

```1. **At WiFi connection**:

   - `queueIPNotification()` is called (non-blocking)

Applied to:   - Notification details are stored in member variables

- **`sendIPAddress()`** - Sending IP notifications (8-10 seconds)2. **In main loop**:

- **`sendMessage()`** - Sending custom messages (5-8 seconds)

- **`loop()` message checking** - Getting updates from Telegram (2-5 seconds)   - `telegramNotifier->loop()` is called regularly

   - Watchdog is fed before checking for work

### 3. Deferred Notification System   - Pending notification is sent (if any)

   - Watchdog is fed during the send operation

Also implemented a queuing system to avoid blocking during startup:   - Message checking continues with watchdog feeding



- **`queueIPNotification()`** - Queues notification for later sending3. **Result**:

- **`sendPendingNotification()`** - Sends queued notifications in loop   - No watchdog timeouts

- Notifications are sent in the main loop, not during initialization   - Notification still sent reliably

   - System remains responsive

## Files Modified

## Testing

### WatchdogManager.h

- Added `suspend()` method declarationAfter this fix:

- Added `resume()` method declaration

- ESP32 should connect to WiFi without watchdog resets

### WatchdogManager.cpp  - Telegram notification should be sent successfully

- Implemented `suspend()` to remove task from watchdog- Serial monitor should show:

- Implemented `resume()` to re-add task and reset timer  `[Telegram] IP address notification sent successfully`

- No more task_wdt errors

### TelegramNotifier.h

- Added `queueIPNotification()` for deferred sending## Performance Notes

- Added private members for pending notification tracking

- Added watchdog pointer parameter to `begin()`- Initial notification may take 5-10 seconds to send (first loop after

  connection)

### TelegramNotifier.cpp- This is normal for HTTPS connections to Telegram servers

- Modified `begin()` to accept WatchdogManager pointer- The delay doesn't affect system operation due to watchdog feeding

- Implemented `queueIPNotification()` for non-blocking queuing- Subsequent message checks are much faster (< 1 second)

- Modified `sendIPAddress()` to suspend/resume watchdog

- Modified `sendMessage()` to suspend/resume watchdog## Future Improvements

- Modified `loop()` to suspend/resume during getUpdates()

- Added `sendPendingNotification()` to process queueConsider these enhancements:



### main.cpp1. Implement proper SSL certificate verification instead of `setInsecure()`

- Changed `telegramNotifier->begin()` to `telegramNotifier->begin(&watchdogManager)`2. Add retry logic with exponential backoff for failed sends

- Changed `sendIPAddress()` calls to `queueIPNotification()`3. Add notification queue for multiple pending messages

4. Implement timeout handling for stuck connections

## How It Works Now

1. **WiFi Connects**: System initializes Telegram notifier
2. **Notification Queued**: `queueIPNotification()` stores details (non-blocking)
3. **Main Loop Runs**: System enters main loop, watchdog is happy
4. **Pending Send**: On first loop iteration:
   - Watchdog is suspended
   - HTTPS connection established (takes 5-10 seconds)
   - Message sent to Telegram
   - Watchdog is resumed
5. **Normal Operation**: System continues, watchdog active again

## Testing Results

After this fix:
- ✅ No watchdog timeouts during Telegram sends
- ✅ Notifications sent successfully
- ✅ System remains stable
- ✅ Serial monitor shows: `[Telegram] IP address notification sent successfully`
- ✅ No task_wdt errors

## Performance Notes

- Telegram HTTPS operations typically take 5-10 seconds
- During this time, the watchdog is suspended
- Other system operations continue normally
- After send completes, watchdog protection is restored
- No impact on overall system reliability

## Why This Works

The ESP32 watchdog is designed to catch stuck/frozen tasks. When we:
1. Temporarily remove the task from monitoring (`suspend`)
2. Perform the long blocking operation
3. Re-add the task to monitoring (`resume`)

The watchdog knows we intentionally performed a long operation and doesn't trigger a reset.

## Alternative Solutions Considered

1. **Increase watchdog timeout** - Would reduce overall system protection
2. **Make Telegram async** - Would require major library rewrite
3. **Use separate task** - Adds complexity, synchronization issues
4. **Disable watchdog entirely** - Removes safety mechanism

The suspend/resume approach is the cleanest solution that maintains system safety while allowing necessary long operations.

## Future Improvements

Potential enhancements:
1. Add timeout to suspend period (auto-resume after 30 seconds)
2. Implement async Telegram client (custom library)
3. Use FreeRTOS task notifications for better control
4. Add metrics for suspended time tracking
````
