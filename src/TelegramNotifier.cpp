#include "TelegramNotifier.h"
#include "WatchdogManager.h"

TelegramNotifier::TelegramNotifier() : lastMessageCheck(0), connectionNotified(false), lastNotifiedIP(""),
                                       pendingNotification(false), pendingIP(""), pendingSSID(""),
                                       notificationQueuedTime(0), watchdog(nullptr)
{
    bot = nullptr;
}

void TelegramNotifier::begin(WatchdogManager *wdt)
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
    watchdog = wdt;

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Initializing Telegram notifier...");
    Serial.printf("[Telegram] Bot Token: %s\n", TELEGRAM_BOT_TOKEN);
    Serial.printf("[Telegram] Chat ID: %s\n", TELEGRAM_CHAT_ID);
    Serial.println("[Telegram]");
    Serial.println("[Telegram] Test your bot configuration:");
    Serial.printf("[Telegram]   https://api.telegram.org/bot%s/getMe\n", TELEGRAM_BOT_TOKEN);
    Serial.println("[Telegram]");
    Serial.println("[Telegram] If bot is valid, send /start to your bot on Telegram!");
#endif

    // Configure WiFiClientSecure to skip SSL certificate verification
    // Note: In production, you should verify certificates for better security
    client.setInsecure();

    // Set connection timeout to 10 seconds
    client.setTimeout(10000);

    // Initialize Telegram bot
    bot = new UniversalTelegramBot(TELEGRAM_BOT_TOKEN, client);

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Telegram notifier initialized");
    Serial.println("[Telegram] Testing connection...");
#endif

    // Test connection by getting bot info (optional)
    // This helps verify credentials early

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Ready to send notifications");
#endif
#endif
}

void TelegramNotifier::loop()
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
    if (bot == nullptr)
    {
        return;
    }

    // Feed watchdog if available
    if (watchdog != nullptr)
    {
        watchdog->feed();
    }

    // Send pending notification first (if any)
    if (pendingNotification)
    {
        sendPendingNotification();
    }

    // Check for new messages periodically
    if (millis() - lastMessageCheck > TELEGRAM_CHECK_INTERVAL)
    {
        // Suspend watchdog during network operations
        if (watchdog != nullptr)
        {
            watchdog->suspend();
        }

        int numNewMessages = bot->getUpdates(bot->last_message_received + 1);

        while (numNewMessages)
        {
            handleNewMessages(numNewMessages);
            numNewMessages = bot->getUpdates(bot->last_message_received + 1);
        }

        // Resume watchdog after network operations
        if (watchdog != nullptr)
        {
            watchdog->resume();
        }

        lastMessageCheck = millis();
    }
#endif
}

void TelegramNotifier::queueIPNotification(const String &ipAddress, const String &ssid)
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
    if (bot == nullptr)
    {
        return;
    }

    // Check if we already notified for this IP
    if (connectionNotified && lastNotifiedIP == ipAddress)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Telegram] Already notified for this connection");
#endif
        return;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Queueing IP address notification...");
#endif

    pendingNotification = true;
    pendingIP = ipAddress;
    pendingSSID = ssid;
    notificationQueuedTime = millis();
#endif
}

void TelegramNotifier::sendIPAddress(const String &ipAddress, const String &ssid)
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
    if (bot == nullptr)
    {
        return;
    }

    // Check if we already notified for this IP
    if (connectionNotified && lastNotifiedIP == ipAddress)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Telegram] Already notified for this connection");
#endif
        return;
    }

    String message = "🌐 ESP32 Connected!\n\n";
    message += "📡 SSID: " + ssid + "\n";
    message += "🔗 IP Address: " + ipAddress + "\n\n";
    message += "You can now access:\n";
    message += "• TCP Server: " + ipAddress + ":" + String(TCP_SERVER_PORT) + "\n";
    message += "• UDP Server: " + ipAddress + ":" + String(UDP_SERVER_PORT) + "\n";
    message += "• Web Interface: http://" + ipAddress + "/\n";
    message += "• mDNS: http://" + String(DEVICE_HOSTNAME) + ".local/";

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Sending IP address notification...");
    Serial.printf("[Telegram] Message length: %d bytes\n", message.length());
    Serial.printf("[Telegram] Target Chat ID: %s\n", TELEGRAM_CHAT_ID);
#endif

    // Suspend watchdog during long blocking HTTPS operation
    if (watchdog != nullptr)
    {
        watchdog->suspend();
    }

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Connecting to Telegram API...");
#endif

    bool success = bot->sendMessage(TELEGRAM_CHAT_ID, message, "");

    // Resume watchdog after operation
    if (watchdog != nullptr)
    {
        watchdog->resume();
    }

    if (success)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Telegram] ✓ IP address notification sent successfully!");
#endif
        connectionNotified = true;
        lastNotifiedIP = ipAddress;
    }
    else
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Telegram] ✗ Failed to send IP address notification");
        Serial.println("[Telegram] Possible issues:");
        Serial.println("[Telegram]   - Check bot token is correct");
        Serial.println("[Telegram]   - Check chat ID is correct");
        Serial.println("[Telegram]   - Verify you've started a chat with the bot");
        Serial.println("[Telegram]   - Check internet connectivity");
        Serial.println("[Telegram]   - Try sending /start to your bot on Telegram");
#endif
    }
#endif
}

void TelegramNotifier::sendMessage(const String &message)
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
    if (bot == nullptr)
    {
        return;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Sending custom message...");
#endif

    // Suspend watchdog during send
    if (watchdog != nullptr)
    {
        watchdog->suspend();
    }

    bot->sendMessage(TELEGRAM_CHAT_ID, message, "");

    // Resume watchdog after send
    if (watchdog != nullptr)
    {
        watchdog->resume();
    }
#endif
}

bool TelegramNotifier::hasNotifiedCurrentConnection()
{
    return connectionNotified;
}

void TelegramNotifier::markConnectionNotified()
{
    connectionNotified = true;
}

void TelegramNotifier::resetNotificationFlag()
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Resetting notification flag");
#endif
    connectionNotified = false;
    lastNotifiedIP = "";
    pendingNotification = false;
}

void TelegramNotifier::sendPendingNotification()
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
    if (!pendingNotification || bot == nullptr)
    {
        return;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.println("[Telegram] Sending pending IP notification...");
#endif

    // Send the notification
    sendIPAddress(pendingIP, pendingSSID);

    // Clear the pending flag
    pendingNotification = false;
    pendingIP = "";
    pendingSSID = "";
#endif
}

void TelegramNotifier::handleNewMessages(int numNewMessages)
{
#if ENABLE_TELEGRAM_NOTIFICATIONS
#if ENABLE_SERIAL_DEBUG
    Serial.printf("[Telegram] Handling %d new messages\n", numNewMessages);
#endif

    for (int i = 0; i < numNewMessages; i++)
    {
        String chat_id = String(bot->messages[i].chat_id);
        String text = bot->messages[i].text;

#if ENABLE_SERIAL_DEBUG
        Serial.printf("[Telegram] Message from %s: %s\n", chat_id.c_str(), text.c_str());
#endif

        String from_name = bot->messages[i].from_name;
        if (from_name == "")
        {
            from_name = "Guest";
        }

        // Only respond to messages from the configured chat ID
        if (chat_id != String(TELEGRAM_CHAT_ID))
        {
#if ENABLE_SERIAL_DEBUG
            Serial.println("[Telegram] Message from unauthorized chat ID, ignoring");
#endif
            continue;
        }

        // Handle commands
        if (text == "/start")
        {
            String welcome = "👋 Welcome to ESP32 Controller!\n\n";
            welcome += "Available commands:\n";
            welcome += "/status - Get current status and IP\n";
            welcome += "/ip - Get IP address\n";
            welcome += "/help - Show this help message";
            bot->sendMessage(chat_id, welcome, "");
        }
        else if (text == "/status" || text == "/ip")
        {
            // This will be sent automatically when connected
            // But user can request it manually
            String response = "ℹ️ Use /start to see available commands";
            bot->sendMessage(chat_id, response, "");
        }
        else if (text == "/help")
        {
            String help = "📱 ESP32 Controller Help\n\n";
            help += "This bot sends you notifications when your ESP32 connects to WiFi.\n\n";
            help += "Commands:\n";
            help += "/start - Show welcome message\n";
            help += "/status - Request current status\n";
            help += "/ip - Request IP address\n";
            help += "/help - Show this help";
            bot->sendMessage(chat_id, help, "");
        }
    }
#endif
}
