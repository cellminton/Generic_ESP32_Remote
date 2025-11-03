# Telegram Connection Troubleshooting Guide

## Issue: "Failed to send IP address notification"

The watchdog suspension is working correctly, but the Telegram API call is
failing. Here are the most common causes and solutions:

## Quick Diagnostics

The serial monitor now shows detailed debug info. Look for these messages:

```
[Telegram] Bot Token: YOUR_TOKEN_HERE
[Telegram] Chat ID: YOUR_CHAT_ID_HERE
[Telegram] Connecting to Telegram API...
[Telegram] ✗ Failed to send IP address notification
```

## Common Issues and Solutions

### 1. **Bot Not Started (Most Common)**

**Problem**: You haven't sent `/start` to your bot yet.

**Solution**:

1. Open Telegram on your phone/desktop
2. Search for your bot (the username you created with BotFather)
3. Click "START" or send the message `/start`
4. Wait for bot to respond
5. Reset your ESP32

**Why**: Telegram requires you to initiate a conversation with a bot before it
can send you messages.

---

### 2. **Wrong Chat ID**

**Problem**: The Chat ID might be incorrect or formatted wrong.

**Solution**:

1. Message your bot with `/start`
2. Visit this URL in your browser (replace YOUR_BOT_TOKEN):
   ```
   https://api.telegram.org/botYOUR_BOT_TOKEN/getUpdates
   ```
3. Look for: `"chat":{"id":123456789`
4. Copy that number (could be negative like `-123456789`)
5. Update `TELEGRAM_CHAT_ID` in Config.h
6. Make sure it's in quotes: `"123456789"` not `123456789`

---

### 3. **Wrong Bot Token**

**Problem**: Bot token is incorrect or has extra spaces.

**Solution**:

1. Go back to BotFather on Telegram
2. Send `/mybots`
3. Select your bot
4. Click "API Token"
5. Copy the FULL token (format: `123456789:ABCdefGHI...`)
6. Make sure there are no spaces before/after in Config.h

---

### 4. **Network/Firewall Issues**

**Problem**: ESP32 can't reach Telegram servers.

**Solution**:

1. Test if your network blocks HTTPS:
   ```cpp
   // Add to setup() temporarily to test
   HTTPClient http;
   http.begin("https://api.telegram.org/");
   int httpCode = http.GET();
   Serial.printf("HTTP Test: %d\n", httpCode);
   ```
2. Try a different WiFi network
3. Check if your router has parental controls blocking Telegram

---

### 5. **SSL/Certificate Issues**

**Problem**: SSL handshake failing.

**Solution**: The code already uses `setInsecure()`, but you can try:

1. Update ESP32 board package in PlatformIO
2. Add this to `begin()`:
   ```cpp
   client.setHandshakeTimeout(30);
   ```

---

### 6. **Bot Token Revoked**

**Problem**: You accidentally revoked the bot token.

**Solution**:

1. Go to BotFather
2. Send `/mybots`
3. Select your bot
4. Select "API Token"
5. Click "Revoke current token" and generate a new one
6. Update Config.h with new token

---

## Testing Your Configuration

### Test 1: Verify Bot Exists

Visit this URL (replace YOUR_BOT_TOKEN):

```
https://api.telegram.org/botYOUR_BOT_TOKEN/getMe
```

**Expected Result**:

```json
{
  "ok": true,
  "result": {
    "id": 123456789,
    "is_bot": true,
    "first_name": "YourBot",
    "username": "yourbot"
  }
}
```

**If you get an error**: Your bot token is wrong.

---

### Test 2: Check Chat History

Visit this URL (replace YOUR_BOT_TOKEN):

```
https://api.telegram.org/botYOUR_BOT_TOKEN/getUpdates
```

**Expected Result**: Should show any messages you've sent to the bot.

**If empty**: You haven't started a chat with the bot yet - send `/start`!

---

### Test 3: Manual Message Send

Visit this URL (replace placeholders):

```
https://api.telegram.org/botYOUR_BOT_TOKEN/sendMessage?chat_id=YOUR_CHAT_ID&text=Test
```

**Expected Result**: You should receive "Test" from your bot.

**If this works but ESP32 doesn't**: It's a network/SSL issue on the ESP32.

---

## Debug Steps

1. **Check Serial Monitor Output**

   - Look for the bot token and chat ID being printed
   - Verify they match your Config.h
   - Make sure there are no extra characters

2. **Verify WiFi Connection**

   - Confirm ESP32 shows "Connected successfully!"
   - Verify IP address is valid
   - Try pinging telegram.org from another device on same network

3. **Test Bot Separately**

   - Use the URLs above to test bot from your computer
   - Make sure bot is active and token is valid
   - Verify you've started a conversation with the bot

4. **Try Simpler Message**

   - Temporarily change the message to just "Test"
   - Some networks block messages with certain characters
   - Emojis might cause issues

5. **Check Heap Memory**
   - The notification shows "Free Heap: 219928 bytes"
   - This is good, memory is not the issue

---

## Still Not Working?

### Option 1: Disable Telegram Temporarily

In Config.h:

```cpp
#define ENABLE_TELEGRAM_NOTIFICATIONS false
```

This lets you use the ESP32 without Telegram while debugging.

---

### Option 2: Use Alternative Notification

Consider these alternatives:

- **Email notifications** (using SMTP)
- **HTTP webhook** to a service you control
- **MQTT** to a broker
- **Pushover** or **Pushbullet** (simpler than Telegram)

---

## Most Likely Solution

**90% of the time**, the issue is:

1. ❌ Haven't sent `/start` to the bot
2. ❌ Wrong chat ID format
3. ❌ Bot token has extra spaces

**Double-check these three things first!**

---

## What's Working

✅ Watchdog suspension - No more resets!  
✅ WiFi connection - ESP32 connects fine  
✅ System stability - Heartbeat shows system healthy  
✅ Code logic - The suspension/resume is perfect

The **only** issue is the Telegram API communication itself.
