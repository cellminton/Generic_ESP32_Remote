#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include <Arduino.h>
#include <esp_task_wdt.h>
#include "Config.h"

/**
 * WatchdogManager - Manages hardware and task watchdog timers
 *
 * Features:
 * - Hardware watchdog timer (HWWDT) for system-level recovery
 * - Task watchdog timer (TWDT) for task monitoring
 * - Automatic system restart on critical failures
 * - Error counting and recovery strategies
 */

class WatchdogManager
{
public:
    WatchdogManager();

    // Initialize watchdog timers
    void begin();

    // Reset/feed the watchdog (call regularly in main loop)
    void feed();

    // Register error
    void registerError(const String &errorMessage);

    // Clear error count
    void clearErrors();

    // Get error count
    int getErrorCount();

    // Check if system should restart
    bool shouldRestart();

    // Perform system restart
    void restart(const String &reason);

    // Get last error message
    String getLastError();

    // Get uptime in seconds
    unsigned long getUptimeSeconds();

    // Get error statistics
    String getErrorStats();

    // Temporarily suspend watchdog monitoring (for long blocking operations)
    void suspend();

    // Resume watchdog monitoring
    void resume();

private:
    unsigned long _lastFeed;
    unsigned long _startTime;
    int _errorCount;
    int _consecutiveErrors;
    String _lastError;
    unsigned long _lastErrorTime;

    bool _hwWatchdogEnabled;
    bool _taskWatchdogEnabled;

    static const unsigned long FEED_INTERVAL = 1000; // Feed every second
};

#endif // WATCHDOG_MANAGER_H
