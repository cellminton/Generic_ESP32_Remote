# ESP32 Controller - Example Scripts

This directory contains example scripts and clients for interacting with the
ESP32 Generic Pin Controller.

## Python Examples

### 1. `python_client.py` - Main Client Library

The core Python library for controlling the ESP32. Can be used as a standalone
script or imported as a module.

**Usage as a script:**

```bash
python python_client.py
```

**Usage as a library:**

```python
from python_client import ESP32Controller

# Auto-discover device
esp = ESP32Controller(auto_discover=True)
esp.connect_tcp()
esp.set_pin(13, 1)
esp.disconnect_tcp()

# Or use manual IP
esp = ESP32Controller("192.168.1.100")
```

**Features:**

- Automatic device discovery (mDNS + UDP broadcast)
- IP address caching for fast reconnection
- Both TCP and UDP protocol support
- All command types: SET, GET, TOGGLE, PWM, STATUS

### 2. `esp32_demo.py` - Comprehensive Demo Script

Interactive demo showcasing all features of the ESP32 controller.

**Usage:**

```bash
python esp32_demo.py
```

**Includes:**

- Basic pin control demonstration
- PWM control with fade effects
- Multiple pin control (Knight Rider pattern)
- TCP vs UDP performance comparison
- Interactive command mode

### 3. `discover_esp32.py` - Device Discovery Tool

Standalone tool to find ESP32 devices on your network.

**Usage:**

```bash
python discover_esp32.py
```

**Features:**

- Tests cached IP first (fastest)
- mDNS hostname resolution
- UDP broadcast scan
- Displays detailed device information

## Other Examples

### `nodejs_client.js` - Node.js Client

Basic Node.js TCP client example.

**Usage:**

```bash
node nodejs_client.js
```

### `test_commands.sh` - Bash Test Script

Shell script for testing commands using netcat.

**Usage:**

```bash
bash test_commands.sh
```

## Quick Start

1. **Find your ESP32:**

   ```bash
   python discover_esp32.py
   ```

2. **Run the demo:**

   ```bash
   python esp32_demo.py
   ```

3. **Use in your own script:**

   ```python
   from python_client import ESP32Controller

   esp = ESP32Controller(auto_discover=True)
   esp.connect_tcp()

   # Your code here
   esp.set_pin(13, 1)

   esp.disconnect_tcp()
   ```

## File Organization

```
examples/
├── python_client.py         # Main Python library (can be imported)
├── esp32_demo.py           # Comprehensive demo script
├── discover_esp32.py       # Device discovery tool
├── nodejs_client.js        # Node.js example
├── test_commands.sh        # Bash/netcat example
└── README.md               # This file
```

## Requirements

**Python scripts require:**

- Python 3.6+
- No external dependencies (uses standard library only)

**Node.js script requires:**

- Node.js (any recent version)
- No external packages

## Troubleshooting

**Device not found:**

- Ensure ESP32 is powered on and connected to WiFi
- Check that your computer is on the same network
- Try disabling Windows Firewall temporarily
- On Linux, ensure UDP broadcast is allowed

**Connection fails:**

- Verify the IP address is correct
- Try pinging the device: `ping <ip_address>`
- Check if ports 8888 (TCP) and 8889 (UDP) are accessible

**Cached IP not working:**

- Delete the cache file: `~/.esp32_last_ip` (Unix) or
  `%USERPROFILE%\.esp32_last_ip` (Windows)
- Run discovery script to find current IP

## Contributing

Feel free to add your own example scripts for different languages or use cases!
