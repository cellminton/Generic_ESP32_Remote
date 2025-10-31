#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// WiFi Configuration
// ============================================================================

// Maximum number of WiFi networks to store
#define MAX_WIFI_NETWORKS 5

// WiFi connection timeout (milliseconds)
#define WIFI_CONNECT_TIMEOUT 10000

// WiFi reconnection attempt interval (milliseconds)
#define WIFI_RECONNECT_INTERVAL 30000

// WiFi credentials - Add your networks here
struct WiFiCredentials
{
    const char *ssid;
    const char *password;
};

// Define your WiFi networks in order of preference
const WiFiCredentials WIFI_NETWORKS[] = {
    {"Meta-Soziales-Netzwerk", "IjWbw21$"},
    {"JUSTUS'", "thisisjustus'wifi"},
    {"nufology", "nufonufo"},
    // Add more networks as needed
};

const int WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WiFiCredentials);

// ============================================================================
// Network Server Configuration
// ============================================================================

// TCP Server port for receiving commands
#define TCP_SERVER_PORT 8888

// UDP Server port for receiving commands
#define UDP_SERVER_PORT 8889

// Maximum number of simultaneous TCP clients
#define MAX_TCP_CLIENTS 4

// Command buffer size
#define COMMAND_BUFFER_SIZE 512

// Response timeout (milliseconds)
#define RESPONSE_TIMEOUT 5000

// ============================================================================
// Pin Configuration
// ============================================================================

// Maximum number of controllable pins
#define MAX_CONTROLLABLE_PINS 32

// Default pins that are safe to control (adjust based on your ESP32 board)
// Avoid pins used for flash (6-11), boot mode (0, 2), and serial (1, 3)
const int SAFE_PINS[] = {
    4, 5, 12, 13, 14, 15, 16, 17, 18, 19,
    21, 22, 23, 25, 26, 27, 32, 33};

const int SAFE_PIN_COUNT = sizeof(SAFE_PINS) / sizeof(int);

// Pin state persistence interval (milliseconds)
// Set to 0 to disable state persistence
#define PIN_STATE_SAVE_INTERVAL 60000

// ============================================================================
// Watchdog Configuration
// ============================================================================

// Hardware watchdog timeout (seconds)
// Must be between 1-4294967 seconds
#define HW_WATCHDOG_TIMEOUT_SEC 8

// Task watchdog timeout (seconds)
#define TASK_WATCHDOG_TIMEOUT_SEC 5

// Enable/disable watchdog timers
#define ENABLE_HW_WATCHDOG true
#define ENABLE_TASK_WATCHDOG true

// ============================================================================
// System Configuration
// ============================================================================

// Device hostname (for mDNS)
#define DEVICE_HOSTNAME "esp32-controller"

// Status LED pin (set to -1 to disable)
#define STATUS_LED_PIN 2

// Status LED blink patterns (milliseconds)
#define LED_BLINK_CONNECTING 500
#define LED_BLINK_CONNECTED 2000
#define LED_BLINK_ERROR 100

// Serial debug output
#define ENABLE_SERIAL_DEBUG true
#define SERIAL_BAUD_RATE 115200

// Heartbeat interval for status messages (milliseconds)
#define HEARTBEAT_INTERVAL 60000

// ============================================================================
// Error Recovery Configuration
// ============================================================================

// Maximum consecutive errors before restart
#define MAX_CONSECUTIVE_ERRORS 10

// Error cooldown period (milliseconds)
#define ERROR_COOLDOWN_PERIOD 5000

// Enable automatic restart on critical errors
#define AUTO_RESTART_ON_CRITICAL_ERROR true

#endif // CONFIG_H
