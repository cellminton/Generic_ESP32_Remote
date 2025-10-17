#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"

/**
 * WiFiManager - Handles WiFi connectivity with multiple network fallback
 *
 * Features:
 * - Multiple WiFi network support with automatic fallback
 * - Automatic reconnection on connection loss
 * - Connection status monitoring
 * - Smart network switching based on signal strength
 */
class WiFiManager
{
public:
    WiFiManager();

    // Initialize WiFi manager
    void begin();

    // Main loop - call regularly to maintain connection
    void loop();

    // Check if connected to WiFi
    bool isConnected();

    // Get current SSID
    String getCurrentSSID();

    // Get current IP address
    String getIPAddress();

    // Get signal strength (RSSI)
    int getSignalStrength();

    // Force reconnection
    void reconnect();

    // Get connection status string
    String getStatusString();

    // Get current network index
    int getCurrentNetworkIndex();

private:
    // Try to connect to a specific network
    bool connectToNetwork(int networkIndex);

    // Try to connect to next available network
    bool connectToNextNetwork();

    // Scan and find best available network
    int findBestNetwork();

    // Check connection health
    void checkConnection();

    int _currentNetworkIndex;
    unsigned long _lastConnectionAttempt;
    unsigned long _lastConnectionCheck;
    bool _isConnected;
    int _consecutiveFailures;

    static const unsigned long CONNECTION_CHECK_INTERVAL = 5000; // 5 seconds
};

#endif // WIFI_MANAGER_H
