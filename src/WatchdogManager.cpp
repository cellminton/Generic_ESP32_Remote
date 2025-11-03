#include "WatchdogManager.h"

WatchdogManager::WatchdogManager()
    : _lastFeed(0),
      _startTime(0),
      _errorCount(0),
      _consecutiveErrors(0),
      _lastError(""),
      _lastErrorTime(0),
      _hwWatchdogEnabled(false),
      _taskWatchdogEnabled(false)
{
}

void WatchdogManager::begin()
{
    _startTime = millis();

#if ENABLE_HW_WATCHDOG
// Enable hardware watchdog
#if ENABLE_SERIAL_DEBUG
    Serial.printf("[WDT] Enabling hardware watchdog (%d seconds)\n", HW_WATCHDOG_TIMEOUT_SEC);
#endif

    esp_task_wdt_init(HW_WATCHDOG_TIMEOUT_SEC, true); // Enable panic
    esp_task_wdt_add(NULL);                           // Add current thread to WDT watch
    _hwWatchdogEnabled = true;
#endif

#if ENABLE_TASK_WATCHDOG
#if ENABLE_SERIAL_DEBUG
    Serial.printf("[WDT] Task watchdog enabled (%d seconds)\n", TASK_WATCHDOG_TIMEOUT_SEC);
#endif
    _taskWatchdogEnabled = true;
#endif

    _lastFeed = millis();

#if ENABLE_SERIAL_DEBUG
    Serial.println("[WDT] Watchdog Manager initialized");
#endif
}

void WatchdogManager::feed()
{
    unsigned long currentMillis = millis();

    // Don't feed too frequently
    if (currentMillis - _lastFeed < FEED_INTERVAL)
    {
        return;
    }

#if ENABLE_HW_WATCHDOG
    if (_hwWatchdogEnabled)
    {
        esp_task_wdt_reset();
    }
#endif

    _lastFeed = currentMillis;

    // Clear consecutive errors after error cooldown period
    if (_consecutiveErrors > 0 &&
        currentMillis - _lastErrorTime > ERROR_COOLDOWN_PERIOD)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[WDT] Error cooldown complete, clearing %d consecutive errors\n",
                      _consecutiveErrors);
#endif
        _consecutiveErrors = 0;
    }
}

void WatchdogManager::registerError(const String &errorMessage)
{
    _errorCount++;
    _consecutiveErrors++;
    _lastError = errorMessage;
    _lastErrorTime = millis();

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[WDT] Error registered: %s (Total: %d, Consecutive: %d)\n",
                  errorMessage.c_str(), _errorCount, _consecutiveErrors);
#endif

    // Check if we should restart
    if (_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[WDT] Maximum consecutive errors reached (%d)\n", _consecutiveErrors);
#endif

#if AUTO_RESTART_ON_CRITICAL_ERROR
        restart("Maximum consecutive errors exceeded");
#endif
    }
}

void WatchdogManager::clearErrors()
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("[WDT] Clearing error count");
#endif

    _consecutiveErrors = 0;
}

int WatchdogManager::getErrorCount()
{
    return _errorCount;
}

bool WatchdogManager::shouldRestart()
{
    return _consecutiveErrors >= MAX_CONSECUTIVE_ERRORS;
}

void WatchdogManager::restart(const String &reason)
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("========================================");
    Serial.printf("[WDT] SYSTEM RESTART INITIATED\n");
    Serial.printf("[WDT] Reason: %s\n", reason.c_str());
    Serial.printf("[WDT] Uptime: %lu seconds\n", getUptimeSeconds());
    Serial.printf("[WDT] Total errors: %d\n", _errorCount);
    Serial.printf("[WDT] Last error: %s\n", _lastError.c_str());
    Serial.println("========================================");
    Serial.flush();
#endif

    delay(1000); // Give time for serial output

    ESP.restart();
}

String WatchdogManager::getLastError()
{
    return _lastError;
}

unsigned long WatchdogManager::getUptimeSeconds()
{
    return (millis() - _startTime) / 1000;
}

String WatchdogManager::getErrorStats()
{
    String stats = "Error Statistics:\n";
    stats += "  Total Errors: " + String(_errorCount) + "\n";
    stats += "  Consecutive Errors: " + String(_consecutiveErrors) + "\n";
    stats += "  Last Error: " + (_lastError.length() > 0 ? _lastError : "None") + "\n";

    if (_lastErrorTime > 0)
    {
        unsigned long timeSinceError = (millis() - _lastErrorTime) / 1000;
        stats += "  Time Since Last Error: " + String(timeSinceError) + " seconds\n";
    }

    stats += "  Uptime: " + String(getUptimeSeconds()) + " seconds\n";
    stats += "  HW Watchdog: " + String(_hwWatchdogEnabled ? "Enabled" : "Disabled") + "\n";
    stats += "  Task Watchdog: " + String(_taskWatchdogEnabled ? "Enabled" : "Disabled") + "\n";

    return stats;
}

void WatchdogManager::suspend()
{
#if ENABLE_HW_WATCHDOG
    if (_hwWatchdogEnabled)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[WDT] Temporarily suspending watchdog monitoring");
#endif
        // Remove current task from watchdog monitoring
        esp_task_wdt_delete(NULL);
    }
#endif
}

void WatchdogManager::resume()
{
#if ENABLE_HW_WATCHDOG
    if (_hwWatchdogEnabled)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[WDT] Resuming watchdog monitoring");
#endif
        // Re-add current task to watchdog monitoring
        esp_task_wdt_add(NULL);
        esp_task_wdt_reset();
        _lastFeed = millis();
    }
#endif
}
