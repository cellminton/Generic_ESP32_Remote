# Telegram Integration Setup Guide

This guide will help you configure Telegram notifications for your ESP32
controller.

## Overview

The ESP32 will automatically send you a Telegram message with its IP address
whenever it successfully connects to WiFi. This is useful for remote access when
the ESP32 gets a dynamic IP address.

## Prerequisites

- A Telegram account
- The Telegram mobile or desktop app

## Step 1: Create a Telegram Bot

1. Open Telegram and search for `@BotFather`
2. Start a chat with BotFather and send the command `/newbot`
3. Follow the prompts to choose a name and username for your bot
4. BotFather will provide you with a **Bot Token** (looks like:
   `123456789:ABCdefGHIjklMNOpqrsTUVwxyz`)
5. **Save this token** - you'll need it in Step 3

## Step 2: Get Your Chat ID

1. Search for `@userinfobot` in Telegram
2. Start a chat with userinfobot
3. The bot will reply with your user information, including your **Chat ID** (a
   number like: `123456789`)
4. **Save this Chat ID** - you'll need it in Step 3

Alternatively, you can:

1. Send any message to your newly created bot
2. Visit: `https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates` (replace
   `<YOUR_BOT_TOKEN>` with your actual token)
3. Look for the `"chat":{"id":` field in the JSON response

## Step 3: Configure Config.h

1. Open `include/Config.h` in your project
2. Find the Telegram configuration section (around line 118)
3. Replace the placeholder values with your actual credentials:

```cpp
// Telegram Bot Token (get from @BotFather on Telegram)
#define TELEGRAM_BOT_TOKEN "123456789:ABCdefGHIjklMNOpqrsTUVwxyz"

// Telegram Chat ID (get by messaging @userinfobot on Telegram)
#define TELEGRAM_CHAT_ID "123456789"
```

## Step 4: Build and Upload

1. Save your changes to `Config.h`
2. Build and upload the firmware to your ESP32
3. The ESP32 will automatically send you a Telegram message when it connects to
   WiFi

## Features

### Automatic IP Notification

When the ESP32 connects to WiFi (either at startup or after a reconnection), it
will send you a message like:

```
🌐 ESP32 Connected!

📡 SSID: YourWiFiName
🔗 IP Address: 192.168.1.100

You can now access:
• TCP Server: 192.168.1.100:8888
• UDP Server: 192.168.1.100:8889
• Web Interface: http://192.168.1.100/
• mDNS: http://esp32-controller.local/
```

### Bot Commands

You can also interact with your bot using these commands:

- `/start` - Show welcome message and available commands
- `/status` - Request current status information
- `/ip` - Request IP address
- `/help` - Show help message

## Troubleshooting

### Not Receiving Messages?

1. **Check your Bot Token**: Make sure you copied it correctly from BotFather
2. **Check your Chat ID**: Verify it's correct by messaging @userinfobot again
3. **Start a conversation**: Send any message to your bot first (like `/start`)
4. **Check Serial Monitor**: Enable `ENABLE_SERIAL_DEBUG` in Config.h and check
   for Telegram-related messages

### Bot Token Format

The bot token should look like: `123456789:ABCdefGHIjklMNOpqrsTUVwxyz`

- First part is numbers
- Followed by a colon `:`
- Then a mix of letters and numbers

### Chat ID Format

The chat ID is typically a string of numbers like: `123456789` or `-123456789`
(can be negative for group chats)

## Disabling Telegram Notifications

If you want to disable Telegram notifications temporarily:

1. Open `include/Config.h`
2. Find the line: `#define ENABLE_TELEGRAM_NOTIFICATIONS true`
3. Change it to: `#define ENABLE_TELEGRAM_NOTIFICATIONS false`
4. Rebuild and upload

## Security Note

⚠️ **Important**: The current implementation uses `client.setInsecure()` which
skips SSL certificate verification. This is acceptable for local networks, but
for production use, you should implement proper certificate verification.

To improve security:

- Store your bot token securely (not in version control)
- Consider using environment variables or separate config files
- Implement proper SSL certificate verification in a production environment

## Additional Resources

- [Telegram Bot API Documentation](https://core.telegram.org/bots/api)
- [UniversalTelegramBot Library](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
- [BotFather Commands](https://core.telegram.org/bots#6-botfather)
