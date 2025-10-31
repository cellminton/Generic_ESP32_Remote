/**
 * ESP32 Generic Pin Controller
 *
 * A robust, modular system for controlling ESP32 GPIO pins via WiFi
 *
 * Features:
 * - Multiple WiFi network support with automatic fallback
 * - TCP and UDP command servers
 * - JSON and text-based command protocols
 * - Digital and PWM pin control
 * - Hardware and task watchdog timers
 * - Automatic error recovery
 * - Comprehensive status reporting
 *
 * Author: Generated for ESP32 Generic Control
 * Date: 2025
 */

#include <Arduino.h>
#include "Config.h"
#include "WiFiManager.h"
#include "WatchdogManager.h"
#include "CommandParser.h"
#include "PinController.h"
#include "NetworkServer.h"
#include "SerialCommandHandler.h"

// Global instances
WiFiManager wifiManager;
WatchdogManager watchdogManager;
CommandParser commandParser;
PinController pinController;
NetworkServer *networkServer = nullptr;
SerialCommandHandler *serialHandler = nullptr;

// Status LED control
unsigned long lastLEDBlink = 0;
int ledState = LOW;
int ledBlinkInterval = LED_BLINK_CONNECTING;

// Heartbeat for periodic status updates
unsigned long lastHeartbeat = 0;

// Restart flag (set by RESET command)
bool restartRequested = false;
unsigned long restartTime = 0;

// Restart callback for serial command handler
void requestRestart(unsigned long delayMs)
{
    restartRequested = true;
    restartTime = millis() + delayMs;
}

void setup()
{
// Initialize serial communication
#if ENABLE_SERIAL_DEBUG
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 3000)
        ; // Wait up to 3 seconds for serial
    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP32 Generic Pin Controller");
    Serial.println("========================================");
    Serial.println();
#endif

// Initialize status LED
#if STATUS_LED_PIN >= 0
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
#endif

// Initialize watchdog manager
#if ENABLE_SERIAL_DEBUG
    Serial.println("[Main] Initializing Watchdog Manager...");
#endif
    watchdogManager.begin();

// Initialize pin controller
#if ENABLE_SERIAL_DEBUG
    Serial.println("[Main] Initializing Pin Controller...");
#endif
    pinController.begin();

// Initialize serial command handler
#if ENABLE_SERIAL_DEBUG
    Serial.println("[Main] Initializing Serial Command Handler...");
#endif
    serialHandler = new SerialCommandHandler(commandParser, pinController, wifiManager, watchdogManager);
    serialHandler->setRestartCallback(requestRestart);

// Initialize WiFi manager
#if ENABLE_SERIAL_DEBUG
    Serial.println("[Main] Initializing WiFi Manager...");
#endif
    wifiManager.begin();

    // Wait a bit for WiFi to connect
    unsigned long wifiStartTime = millis();
    while (!wifiManager.isConnected() && millis() - wifiStartTime < WIFI_CONNECT_TIMEOUT)
    {
        wifiManager.loop();
        watchdogManager.feed();
        delay(100);
    }

    // Initialize network server (only if WiFi is connected)
    if (wifiManager.isConnected())
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Main] Initializing Network Server...");
#endif

        networkServer = new NetworkServer(commandParser, pinController);
        networkServer->begin();

        ledBlinkInterval = LED_BLINK_CONNECTED;

#if ENABLE_SERIAL_DEBUG
        Serial.println();
        Serial.println("========================================");
        Serial.println("  System Ready!");
        Serial.println("========================================");
        Serial.println(wifiManager.getStatusString());
        Serial.printf("TCP Server: %s:%d\n", wifiManager.getIPAddress().c_str(), TCP_SERVER_PORT);
        Serial.printf("UDP Server: %s:%d\n", wifiManager.getIPAddress().c_str(), UDP_SERVER_PORT);
        Serial.println("========================================");
        Serial.println();
#endif
    }
    else
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Main] WARNING: WiFi not connected, server not started");
        Serial.println("[Main] Will retry connection in background");
#endif

        ledBlinkInterval = LED_BLINK_ERROR;
        watchdogManager.registerError("Initial WiFi connection failed");
    }
}

void loop()
{
    // Feed the watchdog
    watchdogManager.feed();

    // Handle WiFi connection
    wifiManager.loop();

    // Initialize server if WiFi just connected and server not yet started
    if (wifiManager.isConnected() && networkServer == nullptr)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Main] WiFi connected, starting server...");
#endif

        networkServer = new NetworkServer(commandParser, pinController);
        networkServer->begin();
        ledBlinkInterval = LED_BLINK_CONNECTED;
        watchdogManager.clearErrors();

#if ENABLE_SERIAL_DEBUG
        Serial.println(wifiManager.getStatusString());
        Serial.printf("TCP Server: %s:%d\n", wifiManager.getIPAddress().c_str(), TCP_SERVER_PORT);
        Serial.printf("UDP Server: %s:%d\n", wifiManager.getIPAddress().c_str(), UDP_SERVER_PORT);
#endif
    }

    // Handle network server if WiFi is connected
    if (wifiManager.isConnected() && networkServer != nullptr)
    {
        networkServer->loop();
        ledBlinkInterval = LED_BLINK_CONNECTED;
    }
    else
    {
        ledBlinkInterval = LED_BLINK_ERROR;

        // Clean up server if WiFi disconnected
        if (!wifiManager.isConnected() && networkServer != nullptr)
        {
#if ENABLE_SERIAL_DEBUG
            Serial.println("[Main] WiFi disconnected, stopping server...");
#endif
            delete networkServer;
            networkServer = nullptr;
        }
    }

    // Handle serial commands
    if (serialHandler != nullptr)
    {
        serialHandler->processSerialCommands();
    }

// Handle status LED
#if STATUS_LED_PIN >= 0
    unsigned long currentMillis = millis();
    if (currentMillis - lastLEDBlink >= ledBlinkInterval)
    {
        lastLEDBlink = currentMillis;
        ledState = (ledState == LOW) ? HIGH : LOW;
        digitalWrite(STATUS_LED_PIN, ledState);
    }
#endif

// Periodic heartbeat
#if ENABLE_SERIAL_DEBUG && HEARTBEAT_INTERVAL > 0
    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL)
    {
        lastHeartbeat = millis();
        Serial.println();
        Serial.println("--- Heartbeat ---");
        Serial.printf("Uptime: %lu seconds\n", watchdogManager.getUptimeSeconds());
        Serial.println(wifiManager.getStatusString());
        if (networkServer != nullptr)
        {
            Serial.printf("TCP Clients: %d\n", networkServer->getConnectedClients());
        }
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.println("-----------------");
        Serial.println();
    }
#endif

    // Handle restart request (from RESET command)
    if (restartRequested && millis() >= restartTime)
    {
        watchdogManager.restart("User requested restart");
    }

    // Check if watchdog manager recommends restart
    if (watchdogManager.shouldRestart())
    {
        watchdogManager.restart("Automatic restart due to errors");
    }

    // Small delay to prevent tight loop
    delay(10);
}
