#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "CommandParser.h"
#include "PinController.h"

/**
 * NetworkServer - Handles TCP and UDP servers for receiving commands
 *
 * Features:
 * - TCP server for reliable command delivery
 * - UDP server for fast, connectionless commands
 * - Multiple simultaneous TCP client support
 * - Command processing and response generation
 */

class NetworkServer
{
public:
    NetworkServer(CommandParser &parser, PinController &pinController);

    // Initialize servers
    void begin();

    // Main loop - call regularly to handle clients and commands
    void loop();

    // Get server status
    String getStatus();

    // Get number of connected TCP clients
    int getConnectedClients();

private:
    // Handle TCP clients
    void handleTCPClients();

    // Handle UDP packets
    void handleUDP();

    // Process a command and generate response
    String processCommand(const String &commandStr);

    // Generate status response
    String generateStatusResponse();

    CommandParser &_parser;
    PinController &_pinController;

    WiFiServer _tcpServer;
    WiFiClient _tcpClients[MAX_TCP_CLIENTS];
    WiFiUDP _udp;

    unsigned long _lastClientCheck;
    static const unsigned long CLIENT_CHECK_INTERVAL = 1000;
};

#endif // NETWORK_SERVER_H
