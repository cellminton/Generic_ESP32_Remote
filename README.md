# ESP32 Generic Pin Controller

A robust, modular, and resilient system for controlling ESP32 GPIO pins via WiFi
commands. This project provides a complete solution for remote pin control with
extensive error handling, multiple network support, and an intuitive command
structure.

## Features

### 🌐 Network Connectivity

- **Multiple WiFi Networks**: Configure up to 5 WiFi networks with automatic
  fallback
- **Smart Network Selection**: Automatically selects the strongest available
  network
- **Auto-Reconnection**: Automatic reconnection on connection loss
- **Dual Protocol Support**: TCP (reliable) and UDP (fast) servers

### 🎮 Pin Control

- **Digital Control**: Set pins HIGH/LOW with simple commands
- **PWM Support**: 8-bit PWM control (0-255) on all GPIO pins
- **State Tracking**: Maintains pin states across operations
- **Safe Pin Configuration**: Predefined safe pins to avoid boot issues

### 🛡️ Robustness & Resilience

- **Hardware Watchdog Timer**: System-level protection against crashes
- **Task Watchdog Timer**: Monitor task execution
- **Error Recovery**: Automatic error counting and recovery
- **Auto-Restart**: Configurable automatic restart on critical errors
- **Status LED**: Visual indication of system state

### 💬 Intuitive Commands

- **Dual Format Support**: JSON and simple text commands
- **Self-Documenting**: Built-in HELP command
- **Comprehensive Responses**: JSON responses with success/failure status

## Quick Start

### 1. Hardware Setup

- ESP32 development board
- USB cable for programming
- (Optional) LED on GPIO 2 for status indication

### 2. Software Requirements

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- ESP32 board support

### 3. Configuration

Edit `include/Config.h` and update your WiFi credentials:

```cpp
const WiFiCredentials WIFI_NETWORKS[] = {
    {"YourPrimarySSID", "YourPrimaryPassword"},
    {"YourSecondarySSID", "YourSecondaryPassword"},
    {"YourTertiarySSID", "YourTertiaryPassword"},
};
```

### 4. Upload

Using PlatformIO:

```bash
pio run --target upload
pio device monitor
```

Using Arduino IDE:

1. Open `src/main.cpp`
2. Select your ESP32 board
3. Click Upload

### 5. Find Your Device

After upload, check the Serial Monitor (115200 baud) for the device IP address:

```
========================================
  System Ready!
========================================
Connected to YourSSID (IP: 192.168.1.100, RSSI: -45 dBm)
TCP Server: 192.168.1.100:8888
UDP Server: 192.168.1.100:8889
========================================
```

## Command Reference

### JSON Format

#### Set Pin Digital

```json
{ "cmd": "SET", "pin": 13, "value": 1 }
```

- `pin`: GPIO pin number
- `value`: 0 (LOW) or 1 (HIGH)

#### Get Pin State

```json
{ "cmd": "GET", "pin": 13 }
```

#### Toggle Pin

```json
{ "cmd": "TOGGLE", "pin": 13 }
```

#### Set PWM

```json
{ "cmd": "PWM", "pin": 13, "value": 128 }
```

- `value`: 0-255 (duty cycle)

#### Get System Status

```json
{ "cmd": "STATUS" }
```

#### Restart System

```json
{ "cmd": "RESET" }
```

#### Get Help

```json
{ "cmd": "HELP" }
```

### Text Format

```
SET 13 1        # Set pin 13 HIGH
GET 13          # Get pin 13 state
TOGGLE 13       # Toggle pin 13
PWM 13 128      # Set PWM on pin 13 to 128
STATUS          # Get system status
RESET           # Restart system
HELP            # Get command help
```

## Client Examples

### Python Client with Auto-Discovery

The Python client (`examples/python_client.py`) includes automatic device
discovery:

#### Quick Start - Auto-Discovery

```python
from python_client import ESP32Controller

# Automatically find and connect to ESP32
esp = ESP32Controller(auto_discover=True)

# Use the controller
esp.connect_tcp()
esp.set_pin(13, 1)
status = esp.get_status()
print(f"Device IP: {status['wifi']['ip']}")
esp.disconnect_tcp()
```

#### Manual IP Address

```python
# Connect using known IP address
esp = ESP32Controller("192.168.1.100")
esp.connect_tcp()
# ... use controller
esp.disconnect_tcp()
```

#### Discovery Methods

The Python client supports automatic IP caching and multiple discovery methods:

**Discovery Priority:**

1. **Cached IP** (fastest): Tries last known IP first
2. **mDNS Resolution**: Fast lookup using hostname
3. **UDP Broadcast Scan**: Network-wide scan (most reliable)

The client automatically saves the last successful IP to `~/.esp32_last_ip`,
making subsequent connections near-instant.

**Manual Discovery Methods:**

1. **mDNS Resolution** (if supported):

```python
ip = ESP32Controller.discover_mdns("esp32-controller.local")
```

2. **UDP Broadcast Scan** (most reliable):

```python
devices = ESP32Controller.discover_devices(timeout=3.0)
for ip, info in devices:
    print(f"Found ESP32 at {ip}")
```

3. **Combined Auto-Discovery** (recommended):

```python
ip = ESP32Controller.find_esp32()  # Tries cached IP, mDNS, then UDP broadcast
```

4. **Disable cache** (force new scan):

```python
ip = ESP32Controller.find_esp32(use_cache=False)
```

#### Standalone Discovery Tool

Use the discovery tool to find devices on your network:

```bash
python examples/discover_esp32.py
```

#### Demo Script

For comprehensive examples of all features, run the demo script:

```bash
python examples/esp32_demo.py
```

The demo includes:

- Basic pin control (set, get, toggle)
- PWM control with fade effects
- Multiple pin control and patterns
- TCP vs UDP performance comparison
- Interactive command mode

Output example:

```
============================================================
 ESP32 Device Discovery Tool
============================================================

[1] Trying cached IP (192.168.1.100)...
    ✓ Cached IP still valid!

    Device available at: 192.168.1.100
============================================================
```

Or if no cached IP exists:

```
============================================================
 ESP32 Device Discovery Tool
============================================================

[1] Trying cached IP (192.168.1.100)...
    ✗ Cached IP no longer responds

[2] Trying mDNS resolution (esp32-controller.local)...
    ✗ mDNS resolution failed

[3] Performing UDP broadcast scan...
    Listening for responses (3 seconds)...

    ✓ Found 1 device(s):

    Device #1: 192.168.1.100
    ──────────────────────────────────────────────────
      Uptime:        3600 seconds
      Free Heap:     245678 bytes
      Chip Model:    ESP32-D0WDQ6
      WiFi SSID:     YourNetwork
      WiFi RSSI:     -45 dBm
      MAC Address:   AA:BB:CC:DD:EE:FF

============================================================
```

## Usage Examples

### Using TCP (Telnet)

```bash
# Connect to the device
telnet 192.168.1.100 8888

# Send commands
SET 13 1
GET 13
TOGGLE 13
PWM 25 200
STATUS
```

### Using Python (TCP)

```python
import socket
import json

# Connect to ESP32
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('192.168.1.100', 8888))

# Send JSON command
command = {"cmd": "SET", "pin": 13, "value": 1}
sock.send((json.dumps(command) + '\n').encode())

# Read response
response = sock.recv(1024).decode()
print(response)

sock.close()
```

### Using Python (UDP)

```python
import socket
import json

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Send command
command = {"cmd": "SET", "pin": 13, "value": 1}
sock.sendto((json.dumps(command)).encode(), ('192.168.1.100', 8889))

# Receive response
response, addr = sock.recvfrom(1024)
print(response.decode())

sock.close()
```

### Using curl (HTTP-style)

```bash
# Note: For actual HTTP support, you'd need to add an HTTP server
# This is a TCP example using netcat
echo '{"cmd":"SET","pin":13,"value":1}' | nc 192.168.1.100 8888
```

### Using Node.js

```javascript
const net = require('net');

const client = new net.Socket();
client.connect(8888, '192.168.1.100', () => {
  console.log('Connected');

  // Send command
  const command = JSON.stringify({ cmd: 'SET', pin: 13, value: 1 });
  client.write(command + '\n');
});

client.on('data', (data) => {
  console.log('Response: ' + data);
  client.destroy();
});

client.on('close', () => {
  console.log('Connection closed');
});
```

## Response Format

All commands return JSON responses:

### Success Response

```json
{
  "success": true,
  "command": "SET",
  "pin": 13,
  "value": 1,
  "message": "Pin set successfully"
}
```

### Error Response

```json
{
  "success": false,
  "command": "SET",
  "message": "Invalid pin number: 99"
}
```

### Status Response

```json
{
  "success": true,
  "command": "STATUS",
  "system": {
    "uptime": 3600,
    "freeHeap": 245678,
    "chipModel": "ESP32-D0WDQ6",
    "cpuFreq": 240
  },
  "wifi": {
    "connected": true,
    "ssid": "YourSSID",
    "ip": "192.168.1.100",
    "rssi": -45
  },
  "server": {
    "tcpPort": 8888,
    "udpPort": 8889,
    "tcpClients": 2
  },
  "watchdog": {
    "errorCount": 0,
    "lastError": ""
  }
}
```

## Configuration Options

All configuration is in `include/Config.h`:

### WiFi Settings

- `MAX_WIFI_NETWORKS`: Maximum number of networks (default: 5)
- `WIFI_CONNECT_TIMEOUT`: Connection timeout in ms (default: 10000)
- `WIFI_RECONNECT_INTERVAL`: Reconnection interval in ms (default: 30000)

### Server Settings

- `TCP_SERVER_PORT`: TCP server port (default: 8888)
- `UDP_SERVER_PORT`: UDP server port (default: 8889)
- `MAX_TCP_CLIENTS`: Maximum simultaneous TCP clients (default: 4)

### Pin Settings

- `SAFE_PINS[]`: Array of safe GPIO pins to use

### Watchdog Settings

- `HW_WATCHDOG_TIMEOUT_SEC`: Hardware watchdog timeout (default: 8)
- `TASK_WATCHDOG_TIMEOUT_SEC`: Task watchdog timeout (default: 5)
- `ENABLE_HW_WATCHDOG`: Enable/disable hardware watchdog (default: true)
- `ENABLE_TASK_WATCHDOG`: Enable/disable task watchdog (default: true)

### Error Recovery

- `MAX_CONSECUTIVE_ERRORS`: Max errors before restart (default: 10)
- `ERROR_COOLDOWN_PERIOD`: Error cooldown time in ms (default: 5000)
- `AUTO_RESTART_ON_CRITICAL_ERROR`: Auto-restart on critical errors (default:
  true)

### Status LED

- `STATUS_LED_PIN`: GPIO pin for status LED (default: 2, set to -1 to disable)
- `LED_BLINK_CONNECTING`: Blink rate when connecting (default: 500ms)
- `LED_BLINK_CONNECTED`: Blink rate when connected (default: 2000ms)
- `LED_BLINK_ERROR`: Blink rate on error (default: 100ms)

## Project Structure

```
Generic_Control/
├── include/
│   ├── Config.h              # Configuration settings
│   ├── WiFiManager.h         # WiFi management
│   ├── WatchdogManager.h     # Watchdog timers
│   ├── CommandParser.h       # Command parsing
│   ├── PinController.h       # Pin control
│   ├── NetworkServer.h       # TCP/UDP servers
│   └── SerialCommandHandler.h # Serial command handling
├── src/
│   ├── main.cpp              # Main application
│   ├── WiFiManager.cpp
│   ├── WatchdogManager.cpp
│   ├── CommandParser.cpp
│   ├── PinController.cpp
│   ├── NetworkServer.cpp
│   └── SerialCommandHandler.cpp
├── examples/
│   ├── python_client.py      # Python client with auto-discovery
│   ├── discover_esp32.py     # Network discovery tool
│   ├── nodejs_client.js      # Node.js example client
│   └── test_commands.sh      # Bash test script
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Safe Pins (Default Configuration)

The following GPIO pins are configured as safe by default:

- **4, 5, 12, 13, 14, 15, 16, 17, 18, 19**
- **21, 22, 23, 25, 26, 27, 32, 33**

**Avoided pins:**

- GPIO 0, 2: Boot mode selection
- GPIO 1, 3: Serial TX/RX
- GPIO 6-11: Connected to flash memory
- GPIO 34-39: Input only (can be added if needed)

## LED Status Indicators

If `STATUS_LED_PIN` is configured (default: GPIO 2):

- **Slow blink (2s)**: Connected and ready
- **Medium blink (500ms)**: Connecting to WiFi
- **Fast blink (100ms)**: Error state

## Troubleshooting

### Device Won't Connect to WiFi

1. Verify WiFi credentials in `Config.h`
2. Check serial output for connection attempts
3. Ensure WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
4. Check WiFi signal strength

### Can't Connect to TCP/UDP Server

1. Verify device IP address from serial output
2. Check firewall settings on your computer
3. Ensure device and client are on the same network
4. Try pinging the device IP

### Pin Doesn't Respond

1. Check if pin is in the `SAFE_PINS` array
2. Verify pin number is correct for your board
3. Ensure pin isn't used for other functions (flash, serial, etc.)
4. Check pin with a multimeter or LED

### System Keeps Restarting

1. Check `MAX_CONSECUTIVE_ERRORS` setting
2. Review serial output for error messages
3. Ensure power supply is adequate
4. Check for short circuits on controlled pins

## Advanced Features

### Serial Command Interface

You can also send commands via Serial Monitor:

```
SET 13 1
GET 13
STATUS
```

### Watchdog Monitoring

The system automatically monitors:

- WiFi connection health
- Command execution
- System uptime
- Error frequency

### Multiple Client Support

- Up to 4 simultaneous TCP clients
- Unlimited UDP clients
- Each client gets independent command processing

## Security Considerations

⚠️ **Important**: This is a basic implementation without authentication. For
production use, consider:

1. Adding password/token authentication
2. Using encrypted connections (TLS/SSL)
3. Implementing rate limiting
4. Adding IP whitelisting
5. Using HTTPS instead of plain TCP/UDP

## License

This project is provided as-is for educational and development purposes.

## Contributing

Suggestions and improvements are welcome! Please ensure any modifications
maintain the modular architecture and robust error handling.

## Support

For issues or questions:

1. Check the serial output for error messages
2. Verify configuration in `Config.h`
3. Review this README
4. Check the PlatformIO/ESP32 documentation

---

**Built with ❤️ for the ESP32 community**
