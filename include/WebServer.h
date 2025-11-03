#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "PinController.h"
#include "Config.h"

/**
 * WebServer - Provides HTTP web interface for ESP32 control
 *
 * Features:
 * - Modern responsive web UI
 * - RESTful API endpoints
 * - Real-time pin control
 * - System status monitoring
 * - Works on desktop and mobile
 */

class WebServer
{
public:
    WebServer(PinController &pinController, uint16_t port = 80);

    // Initialize and start the web server
    void begin();

    // Check if server is running
    bool isRunning() const { return _running; }

    // Get server port
    uint16_t getPort() const { return _port; }

private:
    // Setup all HTTP routes
    void setupRoutes();

    // Route handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleAPI(AsyncWebServerRequest *request);
    void handleSetPin(AsyncWebServerRequest *request);
    void handleGetPin(AsyncWebServerRequest *request);
    void handleTogglePin(AsyncWebServerRequest *request);
    void handleSetPWM(AsyncWebServerRequest *request);
    void handleGetStatus(AsyncWebServerRequest *request);
    void handleResetPins(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);

    // Helper functions
    String getHTMLPage();
    String generatePinControlsHTML();
    void sendJSONResponse(AsyncWebServerRequest *request, int code, bool success, const String &message, const String &data = "");

    AsyncWebServer _server;
    PinController &_pinController;
    uint16_t _port;
    bool _running;
};

#endif // WEB_SERVER_H
