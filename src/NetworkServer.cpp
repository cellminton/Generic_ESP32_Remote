#include "NetworkServer.h"
#include "WiFiManager.h"
#include "WatchdogManager.h"
#include <ArduinoJson.h>

// External references to global instances
extern WiFiManager wifiManager;
extern WatchdogManager watchdogManager;

NetworkServer::NetworkServer(CommandParser &parser, PinController &pinController)
    : _parser(parser),
      _pinController(pinController),
      _tcpServer(TCP_SERVER_PORT),
      _lastClientCheck(0)
{
}

void NetworkServer::begin()
{
    // Start TCP server
    _tcpServer.begin();
    _tcpServer.setNoDelay(true);

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[Server] TCP server started on port %d\n", TCP_SERVER_PORT);
#endif

    // Start UDP server
    if (_udp.begin(UDP_SERVER_PORT))
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[Server] UDP server started on port %d\n", UDP_SERVER_PORT);
#endif
    }
    else
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[Server] Failed to start UDP server");
#endif
    }
}

void NetworkServer::loop()
{
    handleTCPClients();
    handleUDP();
}

void NetworkServer::handleTCPClients()
{
    unsigned long currentMillis = millis();

    // Check for new clients
    if (_tcpServer.hasClient())
    {
        // Find a free slot
        bool slotFound = false;
        for (int i = 0; i < MAX_TCP_CLIENTS; i++)
        {
            if (!_tcpClients[i] || !_tcpClients[i].connected())
            {
                if (_tcpClients[i])
                {
                    _tcpClients[i].stop();
                }
                _tcpClients[i] = _tcpServer.available();

#if ENABLE_SERIAL_DEBUG
                Serial.printf("[Server] New TCP client connected (slot %d)\n", i);
#endif

                // Send welcome message
                _tcpClients[i].println("ESP32 Pin Controller Ready");
                _tcpClients[i].println("Type HELP for command list");

                slotFound = true;
                break;
            }
        }

        if (!slotFound)
        {
            // No free slots, reject the connection
            WiFiClient client = _tcpServer.available();
            client.println("ERROR: Server full");
            client.stop();

#if ENABLE_SERIAL_DEBUG
            Serial.println("[Server] Rejected TCP client (no free slots)");
#endif
        }
    }

    // Handle existing clients
    for (int i = 0; i < MAX_TCP_CLIENTS; i++)
    {
        if (_tcpClients[i] && _tcpClients[i].connected())
        {
            // Check if data is available
            if (_tcpClients[i].available())
            {
                String command = _tcpClients[i].readStringUntil('\n');
                command.trim();

                if (command.length() > 0)
                {
#if ENABLE_SERIAL_DEBUG
                    Serial.printf("[Server] TCP command from client %d: %s\n", i, command.c_str());
#endif

                    String response = processCommand(command);
                    _tcpClients[i].println(response);
                }
            }
        }
        else if (_tcpClients[i])
        {
            // Client disconnected
            _tcpClients[i].stop();

#if ENABLE_SERIAL_DEBUG
            Serial.printf("[Server] TCP client %d disconnected\n", i);
#endif
        }
    }
}

void NetworkServer::handleUDP()
{
    int packetSize = _udp.parsePacket();

    if (packetSize > 0)
    {
        char packet[COMMAND_BUFFER_SIZE];
        int len = _udp.read(packet, COMMAND_BUFFER_SIZE - 1);
        packet[len] = '\0';

        String command = String(packet);
        command.trim();

#if ENABLE_SERIAL_DEBUG
        Serial.printf("[Server] UDP command from %s:%d: %s\n",
                      _udp.remoteIP().toString().c_str(),
                      _udp.remotePort(),
                      command.c_str());
#endif

        String response = processCommand(command);

        // Send response back to sender
        _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
        _udp.print(response);
        _udp.endPacket();
    }
}

String NetworkServer::processCommand(const String &commandStr)
{
    // Parse the command
    Command cmd = _parser.parse(commandStr);

    if (!cmd.isValid())
    {
        return _parser.generateResponse(cmd, false);
    }

    // Execute the command
    bool success = false;
    String message = "";
    int resultValue = -1;

    switch (cmd.type)
    {
    case CommandType::SET:
        success = _pinController.setDigital(cmd.pin, cmd.value);
        message = success ? "Pin set successfully" : "Failed to set pin";
        resultValue = cmd.value;
        break;

    case CommandType::GET:
        resultValue = _pinController.getDigital(cmd.pin);
        success = (resultValue >= 0);
        message = success ? "Pin value retrieved" : "Failed to get pin value";
        break;

    case CommandType::TOGGLE:
        success = _pinController.toggle(cmd.pin);
        if (success)
        {
            resultValue = _pinController.getDigital(cmd.pin);
            message = "Pin toggled successfully";
        }
        else
        {
            message = "Failed to toggle pin";
        }
        break;

    case CommandType::PWM:
        success = _pinController.setPWM(cmd.pin, cmd.value);
        message = success ? "PWM set successfully" : "Failed to set PWM";
        resultValue = cmd.value;
        break;

    case CommandType::STATUS:
        return generateStatusResponse();

    case CommandType::RESET:
        message = "System will restart in 2 seconds";
        success = true;
        // Note: Actual restart will be handled in main loop
        break;

    case CommandType::HELP:
        return _parser.getHelpText();

    default:
        message = "Unknown command";
        success = false;
        break;
    }

    return _parser.generateResponse(cmd, success, message, resultValue);
}

String NetworkServer::generateStatusResponse()
{
    JsonDocument doc;

    doc["success"] = true;
    doc["command"] = "STATUS";

    // System info
    JsonObject system = doc["system"].to<JsonObject>();
    system["uptime"] = watchdogManager.getUptimeSeconds();
    system["free_heap"] = ESP.getFreeHeap();
    system["chip_model"] = ESP.getChipModel();
    system["cpu_freq"] = ESP.getCpuFreqMHz();

    // WiFi info
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["connected"] = wifiManager.isConnected();
    wifi["ssid"] = wifiManager.getCurrentSSID();
    wifi["ip"] = wifiManager.getIPAddress();
    wifi["rssi"] = wifiManager.getSignalStrength();

    // Server info
    JsonObject server = doc["server"].to<JsonObject>();
    server["tcp_port"] = TCP_SERVER_PORT;
    server["udp_port"] = UDP_SERVER_PORT;
    server["tcp_clients"] = getConnectedClients();

    // Pin states
    doc["pin_states"] = _pinController.getStateJSON();

    // Watchdog info
    JsonObject wdt = doc["watchdog"].to<JsonObject>();
    wdt["error_count"] = watchdogManager.getErrorCount();
    wdt["last_error"] = watchdogManager.getLastError();

    String response;
    serializeJsonPretty(doc, response);
    return response;
}

String NetworkServer::getStatus()
{
    String status = "Network Server Status:\n";
    status += "  TCP Server: Port " + String(TCP_SERVER_PORT) + "\n";
    status += "  UDP Server: Port " + String(UDP_SERVER_PORT) + "\n";
    status += "  Connected TCP Clients: " + String(getConnectedClients()) + "\n";
    return status;
}

int NetworkServer::getConnectedClients()
{
    int count = 0;
    for (int i = 0; i < MAX_TCP_CLIENTS; i++)
    {
        if (_tcpClients[i] && _tcpClients[i].connected())
        {
            count++;
        }
    }
    return count;
}
