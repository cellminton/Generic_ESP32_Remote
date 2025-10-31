# JSON Field Naming Convention Fix

## Issue

The STATUS command was failing because of a mismatch in JSON field naming
conventions:

- **ESP32 C++ code** was using `snake_case` (e.g., `free_heap`, `chip_model`)
- **Python client** was expecting `camelCase` (e.g., `freeHeap`, `chipModel`)

## Root Cause

```cpp
// Old (snake_case)
system["free_heap"] = ESP.getFreeHeap();
system["chip_model"] = ESP.getChipModel();
```

```python
# Python expected camelCase
print(f"Free Heap: {status['system']['freeHeap']}")  # KeyError!
```

## Solution

### 1. Updated ESP32 to Use camelCase

Changed `NetworkServer::generateStatusResponse()` to use camelCase for all JSON
fields:

**Before:**

```cpp
system["free_heap"] = ESP.getFreeHeap();
system["chip_model"] = ESP.getChipModel();
system["cpu_freq"] = ESP.getCpuFreqMHz();
wifi["rssi"] = wifiManager.getSignalStrength();
server["tcp_port"] = TCP_SERVER_PORT;
server["tcp_clients"] = getConnectedClients();
doc["pin_states"] = _pinController.getStateJSON();
wdt["error_count"] = watchdogManager.getErrorCount();
wdt["last_error"] = watchdogManager.getLastError();
```

**After:**

```cpp
system["freeHeap"] = ESP.getFreeHeap();
system["chipModel"] = ESP.getChipModel();
system["cpuFreq"] = ESP.getCpuFreqMHz();
wifi["rssi"] = wifiManager.getSignalStrength();
server["tcpPort"] = TCP_SERVER_PORT;
server["tcpClients"] = getConnectedClients();
doc["pinStates"] = _pinController.getStateJSON();
wdt["errorCount"] = watchdogManager.getErrorCount();
wdt["lastError"] = watchdogManager.getLastError();
```

### 2. Made Python Code More Robust

Added fallback support for both naming conventions:

**Before:**

```python
# Would crash if field name changed
print(f"Free Heap: {status['system']['freeHeap']}")
```

**After:**

```python
# Handles both formats gracefully
system = status.get('system', {})
free_heap = system.get('freeHeap', system.get('free_heap', 0))
print(f"Free Heap: {free_heap}")
```

### 3. Updated Documentation

Fixed the README.md to show correct field names in camelCase.

## Why camelCase?

**Standard JSON Convention:**

- JavaScript/JSON typically uses camelCase
- More common in REST APIs and web services
- Easier for JavaScript clients to work with
- Consistent with ArduinoJson library examples

**Alternative conventions:**

- `snake_case` - More common in Python/C++
- `PascalCase` - Used in C#/.NET
- `kebab-case` - Used in URLs/CSS

We chose camelCase because:

1. ✅ JSON standard convention
2. ✅ Works well with JavaScript clients
3. ✅ Consistent across all fields
4. ✅ ArduinoJson library uses it

## Files Modified

1. **`src/NetworkServer.cpp`** - Changed STATUS response to camelCase
2. **`examples/python_client.py`** - Added fallback support
3. **`examples/esp32_demo.py`** - Added robust field access
4. **`README.md`** - Updated documentation

## Testing

After uploading the new ESP32 code, the STATUS command should work:

```bash
python esp32_demo.py
```

Expected output:

```
1. Getting system status...
   Uptime:        3600 seconds
   Free Heap:     245678 bytes
   Chip Model:    ESP32-D0WDQ6
   WiFi SSID:     YourNetwork
   WiFi IP:       192.168.1.100
   WiFi RSSI:     -45 dBm
```

## Backwards Compatibility

The Python code now supports both formats, so it will work with:

- ✅ Old ESP32 firmware (snake_case)
- ✅ New ESP32 firmware (camelCase)

## Upload Instructions

1. Upload the new ESP32 code:

   ```bash
   pio run --target upload
   ```

2. Test the STATUS command:
   ```bash
   python examples/python_client.py
   ```

The STATUS command should now work correctly! 🎉
