#include "WiFiManager.h"
#include "WatchdogManager.h"

// External reference to global watchdog manager
extern WatchdogManager watchdogManager;

WiFiManager::WiFiManager()
    : _currentNetworkIndex(-1),
      _lastConnectionAttempt(0),
      _lastConnectionCheck(0),
      _isConnected(false),
      _consecutiveFailures(0)
{
}

void WiFiManager::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // We'll handle reconnection manually
    WiFi.persistent(false);       // Don't save WiFi config to flash

#if ENABLE_SERIAL_DEBUG
    Serial.println("[WiFi] WiFi Manager initialized");
    Serial.printf("[WiFi] Found %d configured networks\n", WIFI_NETWORK_COUNT);
#endif

    // Try to connect to best available network
    connectToNextNetwork();
}

void WiFiManager::loop()
{
    unsigned long currentMillis = millis();

    // Periodic connection check
    if (currentMillis - _lastConnectionCheck >= CONNECTION_CHECK_INTERVAL)
    {
        _lastConnectionCheck = currentMillis;
        checkConnection();
    }

    // Handle reconnection if disconnected
    if (!_isConnected &&
        (currentMillis - _lastConnectionAttempt >= WIFI_RECONNECT_INTERVAL))
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[WiFi] Attempting to reconnect...");
#endif
        connectToNextNetwork();
    }
}

bool WiFiManager::isConnected()
{
    return _isConnected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getCurrentSSID()
{
    if (_isConnected)
    {
        return WiFi.SSID();
    }
    return "Not connected";
}

String WiFiManager::getIPAddress()
{
    if (_isConnected)
    {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getSignalStrength()
{
    if (_isConnected)
    {
        return WiFi.RSSI();
    }
    return -100;
}

void WiFiManager::reconnect()
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("[WiFi] Manual reconnect requested");
#endif

    WiFi.disconnect();
    _isConnected = false;
    connectToNextNetwork();
}

String WiFiManager::getStatusString()
{
    if (_isConnected)
    {
        return String("Connected to ") + getCurrentSSID() +
               " (IP: " + getIPAddress() +
               ", RSSI: " + String(getSignalStrength()) + " dBm)";
    }
    else
    {
        return "Disconnected";
    }
}

int WiFiManager::getCurrentNetworkIndex()
{
    return _currentNetworkIndex;
}

bool WiFiManager::connectToNetwork(int networkIndex)
{
    if (networkIndex < 0 || networkIndex >= WIFI_NETWORK_COUNT)
    {
        return false;
    }

    const WiFiCredentials &creds = WIFI_NETWORKS[networkIndex];

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[WiFi] Attempting to connect to: %s\n", creds.ssid);
#endif

    WiFi.disconnect();
    delay(100);

    WiFi.begin(creds.ssid, creds.password);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttempt < WIFI_CONNECT_TIMEOUT)
    {
        delay(100);
        // Feed the watchdog during WiFi connection to prevent timeout
        watchdogManager.feed();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        _isConnected = true;
        _currentNetworkIndex = networkIndex;
        _consecutiveFailures = 0;

#if ENABLE_SERIAL_DEBUG
        Serial.printf("[WiFi] Connected successfully!\n");
        Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] Signal Strength: %d dBm\n", WiFi.RSSI());
#endif

        return true;
    }
    else
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[WiFi] Failed to connect to: %s\n", creds.ssid);
#endif
        return false;
    }
}

bool WiFiManager::connectToNextNetwork()
{
    _lastConnectionAttempt = millis();

    // First, try to find the best network by scanning
    int bestNetwork = findBestNetwork();

    if (bestNetwork >= 0)
    {
        if (connectToNetwork(bestNetwork))
        {
            return true;
        }
    }

    // If scan-based connection failed, try each network in order
    for (int i = 0; i < WIFI_NETWORK_COUNT; i++)
    {
        if (connectToNetwork(i))
        {
            return true;
        }
    }

    // All networks failed
    _isConnected = false;
    _consecutiveFailures++;

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[WiFi] All networks failed. Consecutive failures: %d\n",
                  _consecutiveFailures);
#endif

    return false;
}

int WiFiManager::findBestNetwork()
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("[WiFi] Scanning for networks...");
#endif

    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == 0)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[WiFi] No networks found");
#endif
        return -1;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[WiFi] Found %d networks\n", numNetworks);
#endif

    int bestNetwork = -1;
    int bestRSSI = -100;

    // Find the strongest configured network
    for (int i = 0; i < WIFI_NETWORK_COUNT; i++)
    {
        for (int j = 0; j < numNetworks; j++)
        {
            if (WiFi.SSID(j) == String(WIFI_NETWORKS[i].ssid))
            {
                int rssi = WiFi.RSSI(j);
#if ENABLE_SERIAL_DEBUG
                Serial.printf("[WiFi] Found configured network: %s (RSSI: %d dBm)\n",
                              WIFI_NETWORKS[i].ssid, rssi);
#endif

                if (rssi > bestRSSI)
                {
                    bestRSSI = rssi;
                    bestNetwork = i;
                }
            }
        }
    }

    WiFi.scanDelete(); // Clean up scan results

    if (bestNetwork >= 0)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[WiFi] Best network: %s (RSSI: %d dBm)\n",
                      WIFI_NETWORKS[bestNetwork].ssid, bestRSSI);
#endif
    }

    return bestNetwork;
}

void WiFiManager::checkConnection()
{
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

    if (_isConnected && !currentlyConnected)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[WiFi] Connection lost!");
#endif
        _isConnected = false;
    }
    else if (!_isConnected && currentlyConnected)
    {
        // Reconnected (shouldn't normally happen with autoReconnect off)
        _isConnected = true;
#if ENABLE_SERIAL_DEBUG
        Serial.println("[WiFi] Connection restored");
#endif
    }
}
