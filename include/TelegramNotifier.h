#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "Config.h"

// Forward declaration
class WatchdogManager;

/**
 * TelegramNotifier - Handles Telegram notifications
 *
 * Features:
 * - Send IP address notifications when ESP32 connects to WiFi
 * - Receive and respond to Telegram commands
 * - Automatic notification on WiFi connection
 */
class TelegramNotifier
{
public:
    TelegramNotifier();

    // Initialize Telegram notifier with watchdog
    void begin(WatchdogManager *wdt = nullptr);

    // Main loop - call regularly to check for messages
    void loop();

    // Queue IP address notification to be sent in loop (non-blocking)
    void queueIPNotification(const String &ipAddress, const String &ssid);

    // Send IP address notification (blocking - only use if watchdog is fed externally)
    void sendIPAddress(const String &ipAddress, const String &ssid);

    // Send a custom message
    void sendMessage(const String &message);

    // Check if notification was already sent for current connection
    bool hasNotifiedCurrentConnection();

    // Mark that notification has been sent for current connection
    void markConnectionNotified();

    // Reset notification flag (for new connections)
    void resetNotificationFlag();

private:
    WiFiClientSecure client;
    UniversalTelegramBot *bot;
    WatchdogManager *watchdog;
    unsigned long lastMessageCheck;
    bool connectionNotified;
    String lastNotifiedIP;

    // Deferred notification support
    bool pendingNotification;
    String pendingIP;
    String pendingSSID;
    unsigned long notificationQueuedTime;

    // Handle incoming messages
    void handleNewMessages(int numNewMessages);

    // Send the pending notification
    void sendPendingNotification();
};

#endif // TELEGRAM_NOTIFIER_H
