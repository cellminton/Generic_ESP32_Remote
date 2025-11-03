#include "WebServer.h"
#include <ArduinoJson.h>

WebServer::WebServer(PinController &pinController, uint16_t port)
    : _server(port), _pinController(pinController), _port(port), _running(false)
{
}

void WebServer::begin()
{
    setupRoutes();
    _server.begin();
    _running = true;

#if ENABLE_SERIAL_DEBUG
    Serial.print("[WebServer] Started on port ");
    Serial.println(_port);
    Serial.print("[WebServer] Access at: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
#endif
}

void WebServer::setupRoutes()
{
    // Main page
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               { this->handleRoot(request); });

    // API endpoints
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request)
               { this->handleGetStatus(request); });

    _server.on("/api/pin/set", HTTP_POST, [this](AsyncWebServerRequest *request)
               { this->handleSetPin(request); });

    _server.on("/api/pin/get", HTTP_GET, [this](AsyncWebServerRequest *request)
               { this->handleGetPin(request); });

    _server.on("/api/pin/toggle", HTTP_POST, [this](AsyncWebServerRequest *request)
               { this->handleTogglePin(request); });

    _server.on("/api/pin/pwm", HTTP_POST, [this](AsyncWebServerRequest *request)
               { this->handleSetPWM(request); });

    _server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request)
               { this->handleResetPins(request); });

    // 404 handler
    _server.onNotFound([this](AsyncWebServerRequest *request)
                       { this->handleNotFound(request); });
}

void WebServer::handleRoot(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", getHTMLPage());
}

void WebServer::handleGetStatus(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    doc["success"] = true;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["chipModel"] = ESP.getChipModel();
    doc["chipCores"] = ESP.getChipCores();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["uptime"] = millis() / 1000;
    doc["tcpPort"] = 8888;
    doc["udpPort"] = 8889;
    doc["webPort"] = _port;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleSetPin(AsyncWebServerRequest *request)
{
    if (!request->hasParam("pin") || !request->hasParam("value"))
    {
        sendJSONResponse(request, 400, false, "Missing pin or value parameter");
        return;
    }

    int pin = request->getParam("pin")->value().toInt();
    int value = request->getParam("value")->value().toInt();

    if (_pinController.setDigital(pin, value))
    {
        sendJSONResponse(request, 200, true, "Pin set successfully");
    }
    else
    {
        sendJSONResponse(request, 400, false, "Failed to set pin");
    }
}

void WebServer::handleGetPin(AsyncWebServerRequest *request)
{
    if (!request->hasParam("pin"))
    {
        sendJSONResponse(request, 400, false, "Missing pin parameter");
        return;
    }

    int pin = request->getParam("pin")->value().toInt();

    if (!_pinController.isPinConfigured(pin))
    {
        sendJSONResponse(request, 400, false, "Pin not configured");
        return;
    }

    PinMode mode = _pinController.getPinMode(pin);
    int value;

    if (mode == PinMode::PWM_OUTPUT)
    {
        value = _pinController.getPWM(pin);
    }
    else
    {
        value = _pinController.getDigital(pin);
    }

    JsonDocument doc;
    doc["success"] = true;
    doc["pin"] = pin;
    doc["value"] = value;
    doc["mode"] = (mode == PinMode::PWM_OUTPUT) ? "PWM" : "DIGITAL";

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServer::handleTogglePin(AsyncWebServerRequest *request)
{
    if (!request->hasParam("pin"))
    {
        sendJSONResponse(request, 400, false, "Missing pin parameter");
        return;
    }

    int pin = request->getParam("pin")->value().toInt();

    if (_pinController.toggle(pin))
    {
        int newValue = _pinController.getDigital(pin);
        JsonDocument doc;
        doc["success"] = true;
        doc["message"] = "Pin toggled successfully";
        doc["newValue"] = newValue;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else
    {
        sendJSONResponse(request, 400, false, "Failed to toggle pin");
    }
}

void WebServer::handleSetPWM(AsyncWebServerRequest *request)
{
    if (!request->hasParam("pin") || !request->hasParam("value"))
    {
        sendJSONResponse(request, 400, false, "Missing pin or value parameter");
        return;
    }

    int pin = request->getParam("pin")->value().toInt();
    int value = request->getParam("value")->value().toInt();

    if (value < 0 || value > 255)
    {
        sendJSONResponse(request, 400, false, "PWM value must be 0-255");
        return;
    }

    if (_pinController.setPWM(pin, value))
    {
        sendJSONResponse(request, 200, true, "PWM set successfully");
    }
    else
    {
        sendJSONResponse(request, 400, false, "Failed to set PWM");
    }
}

void WebServer::handleResetPins(AsyncWebServerRequest *request)
{
    if (_pinController.resetAllPins())
    {
        sendJSONResponse(request, 200, true, "All pins reset successfully");
    }
    else
    {
        sendJSONResponse(request, 500, false, "Failed to reset pins");
    }
}

void WebServer::handleNotFound(AsyncWebServerRequest *request)
{
    sendJSONResponse(request, 404, false, "Endpoint not found");
}

void WebServer::sendJSONResponse(AsyncWebServerRequest *request, int code, bool success, const String &message, const String &data)
{
    JsonDocument doc;
    doc["success"] = success;
    doc["message"] = message;
    if (data.length() > 0)
    {
        doc["data"] = data;
    }

    String response;
    serializeJson(doc, response);
    request->send(code, "application/json", response);
}

String WebServer::getHTMLPage()
{
    return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Pin Controller</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            color: #333;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        .header {
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            margin-bottom: 30px;
            text-align: center;
        }
        
        .header h1 {
            color: #667eea;
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        .header p {
            color: #666;
            font-size: 1.1em;
        }
        
        .status-bar {
            background: white;
            padding: 20px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            margin-bottom: 30px;
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
        }
        
        .status-item {
            text-align: center;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            border-radius: 10px;
            color: white;
        }
        
        .status-item .label {
            font-size: 0.9em;
            opacity: 0.9;
            margin-bottom: 5px;
        }
        
        .status-item .value {
            font-size: 1.5em;
            font-weight: bold;
        }
        
        .controls {
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            margin-bottom: 30px;
        }
        
        .controls h2 {
            color: #667eea;
            margin-bottom: 20px;
            font-size: 1.8em;
        }
        
        .pin-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .pin-card {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            border: 2px solid #e9ecef;
            transition: all 0.3s ease;
        }
        
        .pin-card:hover {
            border-color: #667eea;
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.2);
        }
        
        .pin-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
        }
        
        .pin-header-left {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .pin-number {
            font-size: 1.5em;
            font-weight: bold;
            color: #667eea;
        }
        
        .pin-mode {
            font-size: 0.9em;
            padding: 5px 10px;
            background: #667eea;
            color: white;
            border-radius: 5px;
        }
        
        .btn-remove {
            background: none;
            border: none;
            color: #dc3545;
            font-size: 1.5em;
            cursor: pointer;
            padding: 5px 10px;
            transition: all 0.3s ease;
            line-height: 1;
        }
        
        .btn-remove:hover {
            color: #c82333;
            transform: scale(1.2);
        }
        
        .button-group {
            display: flex;
            gap: 10px;
            margin-bottom: 15px;
        }
        
        button {
            flex: 1;
            padding: 12px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .btn-on {
            background: #28a745;
            color: white;
        }
        
        .btn-on:hover {
            background: #218838;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(40, 167, 69, 0.3);
        }
        
        .btn-off {
            background: #dc3545;
            color: white;
        }
        
        .btn-off:hover {
            background: #c82333;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(220, 53, 69, 0.3);
        }
        
        .btn-toggle {
            background: #667eea;
            color: white;
        }
        
        .btn-toggle:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.3);
        }
        
        .btn-reset {
            background: #ff6b6b;
            color: white;
            width: 100%;
            padding: 15px;
            font-size: 1.1em;
        }
        
        .btn-reset:hover {
            background: #ee5a52;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(255, 107, 107, 0.3);
        }
        
        .pwm-control {
            margin-top: 15px;
        }
        
        .pwm-slider {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #d3d3d3;
            outline: none;
            -webkit-appearance: none;
            margin-bottom: 10px;
        }
        
        .pwm-slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
        
        .pwm-slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
            border: none;
        }
        
        .pwm-value {
            text-align: center;
            font-size: 1.2em;
            font-weight: bold;
            color: #667eea;
        }
        
        .pin-state {
            text-align: center;
            padding: 10px;
            border-radius: 5px;
            font-weight: bold;
            margin-top: 10px;
        }
        
        .state-high {
            background: #28a745;
            color: white;
        }
        
        .state-low {
            background: #dc3545;
            color: white;
        }
        
        .add-pin {
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
            margin-bottom: 30px;
        }
        
        .add-pin h2 {
            color: #667eea;
            margin-bottom: 20px;
            font-size: 1.8em;
        }
        
        .input-group {
            display: flex;
            gap: 10px;
            margin-bottom: 15px;
        }
        
        input[type="number"] {
            flex: 1;
            padding: 12px;
            border: 2px solid #e9ecef;
            border-radius: 8px;
            font-size: 1em;
            transition: border-color 0.3s ease;
        }
        
        input[type="number"]:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .btn-add {
            background: #667eea;
            color: white;
            padding: 12px 30px;
        }
        
        .btn-add:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.3);
        }
        
        .notification {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
            z-index: 1000;
            display: none;
            animation: slideIn 0.3s ease;
        }
        
        @keyframes slideIn {
            from {
                transform: translateX(400px);
                opacity: 0;
            }
            to {
                transform: translateX(0);
                opacity: 1;
            }
        }
        
        .notification.success {
            border-left: 4px solid #28a745;
        }
        
        .notification.error {
            border-left: 4px solid #dc3545;
        }
        
        @media (max-width: 768px) {
            .header h1 {
                font-size: 1.8em;
            }
            
            .pin-grid {
                grid-template-columns: 1fr;
            }
            
            .status-bar {
                grid-template-columns: repeat(2, 1fr);
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎛️ ESP32 Pin Controller</h1>
            <p>Real-time GPIO control via web interface</p>
        </div>
        
        <div class="status-bar">
            <div class="status-item">
                <div class="label">Free Heap</div>
                <div class="value" id="freeHeap">-</div>
            </div>
            <div class="status-item">
                <div class="label">Uptime</div>
                <div class="value" id="uptime">-</div>
            </div>
            <div class="status-item">
                <div class="label">CPU Freq</div>
                <div class="value" id="cpuFreq">-</div>
            </div>
            <div class="status-item">
                <div class="label">Chip Model</div>
                <div class="value" id="chipModel">-</div>
            </div>
        </div>
        
        <div class="add-pin">
            <h2>Add Pin Control</h2>
            <div class="input-group">
                <input type="number" id="newPin" placeholder="Pin Number (e.g., 13)" min="0" max="39">
                <button class="btn-add" onclick="addPin()">Add Digital Pin</button>
                <button class="btn-add" onclick="addPWMPin()">Add PWM Pin</button>
            </div>
        </div>
        
        <div class="controls">
            <h2>Pin Controls</h2>
            <div class="pin-grid" id="pinGrid">
                <!-- Pin cards will be added dynamically -->
            </div>
            <button class="btn-reset" onclick="resetAllPins()">🔄 Reset All Pins</button>
        </div>
    </div>
    
    <div class="notification" id="notification"></div>
    
    <script>
        const pins = new Set();
        
        function showNotification(message, type = 'success') {
            const notif = document.getElementById('notification');
            notif.textContent = message;
            notif.className = 'notification ' + type;
            notif.style.display = 'block';
            setTimeout(() => {
                notif.style.display = 'none';
            }, 3000);
        }
        
        async function apiCall(url, method = 'GET') {
            try {
                const response = await fetch(url, { method });
                const data = await response.json();
                return data;
            } catch (error) {
                console.error('API call failed:', error);
                showNotification('API call failed: ' + error.message, 'error');
                return null;
            }
        }
        
        async function updateStatus() {
            const data = await apiCall('/api/status');
            if (data && data.success) {
                document.getElementById('freeHeap').textContent = Math.round(data.freeHeap / 1024) + ' KB';
                document.getElementById('uptime').textContent = formatUptime(data.uptime);
                document.getElementById('cpuFreq').textContent = data.cpuFreq + ' MHz';
                document.getElementById('chipModel').textContent = data.chipModel;
            }
        }
        
        function formatUptime(seconds) {
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = seconds % 60;
            return `${hours}h ${minutes}m ${secs}s`;
        }
        
        async function setPin(pin, value) {
            const data = await apiCall(`/api/pin/set?pin=${pin}&value=${value}`, 'POST');
            if (data && data.success) {
                showNotification(`Pin ${pin} set to ${value ? 'HIGH' : 'LOW'}`);
                updatePinState(pin, value);
            } else {
                showNotification('Failed to set pin', 'error');
            }
        }
        
        async function togglePin(pin) {
            const data = await apiCall(`/api/pin/toggle?pin=${pin}`, 'POST');
            if (data && data.success) {
                showNotification(`Pin ${pin} toggled to ${data.newValue ? 'HIGH' : 'LOW'}`);
                updatePinState(pin, data.newValue);
            } else {
                showNotification('Failed to toggle pin', 'error');
            }
        }
        
        async function setPWM(pin, value) {
            const data = await apiCall(`/api/pin/pwm?pin=${pin}&value=${value}`, 'POST');
            if (data && data.success) {
                showNotification(`Pin ${pin} PWM set to ${value}`);
            } else {
                showNotification('Failed to set PWM', 'error');
            }
        }
        
        async function resetAllPins() {
            if (!confirm('Reset all pins to LOW?')) return;
            
            const data = await apiCall('/api/reset', 'POST');
            if (data && data.success) {
                showNotification('All pins reset successfully');
                // Update all pin states
                pins.forEach(pin => updatePinState(pin, 0));
            } else {
                showNotification('Failed to reset pins', 'error');
            }
        }
        
        function updatePinState(pin, value) {
            const stateDiv = document.getElementById(`state-${pin}`);
            if (stateDiv) {
                stateDiv.textContent = value ? 'HIGH' : 'LOW';
                stateDiv.className = 'pin-state ' + (value ? 'state-high' : 'state-low');
            }
        }
        
        function removePin(pin) {
            if (!confirm(`Remove pin ${pin} from interface?`)) return;
            
            const card = document.getElementById(`card-${pin}`);
            if (card) {
                card.remove();
                pins.delete(pin);
                showNotification(`Pin ${pin} removed from interface`);
            }
        }
        
        function addPin() {
            const pinInput = document.getElementById('newPin');
            const pin = parseInt(pinInput.value);
            
            if (isNaN(pin) || pin < 0 || pin > 39) {
                showNotification('Invalid pin number', 'error');
                return;
            }
            
            if (pins.has(pin)) {
                showNotification('Pin already added', 'error');
                return;
            }
            
            pins.add(pin);
            createPinCard(pin, false);
            pinInput.value = '';
            showNotification(`Pin ${pin} added`);
        }
        
        function addPWMPin() {
            const pinInput = document.getElementById('newPin');
            const pin = parseInt(pinInput.value);
            
            if (isNaN(pin) || pin < 0 || pin > 39) {
                showNotification('Invalid pin number', 'error');
                return;
            }
            
            if (pins.has(pin)) {
                showNotification('Pin already added', 'error');
                return;
            }
            
            pins.add(pin);
            createPinCard(pin, true);
            pinInput.value = '';
            showNotification(`PWM Pin ${pin} added`);
        }
        
        function createPinCard(pin, isPWM) {
            const grid = document.getElementById('pinGrid');
            const card = document.createElement('div');
            card.className = 'pin-card';
            card.id = `card-${pin}`;
            
            if (isPWM) {
                card.innerHTML = `
                    <div class="pin-header">
                        <div class="pin-header-left">
                            <div class="pin-number">Pin ${pin}</div>
                            <div class="pin-mode">PWM</div>
                        </div>
                        <button class="btn-remove" onclick="removePin(${pin})" title="Remove pin">✕</button>
                    </div>
                    <div class="pwm-control">
                        <input type="range" class="pwm-slider" min="0" max="255" value="0" 
                               oninput="document.getElementById('pwm-val-${pin}').textContent = this.value; setPWM(${pin}, this.value)">
                        <div class="pwm-value">Value: <span id="pwm-val-${pin}">0</span></div>
                    </div>
                `;
            } else {
                card.innerHTML = `
                    <div class="pin-header">
                        <div class="pin-header-left">
                            <div class="pin-number">Pin ${pin}</div>
                            <div class="pin-mode">DIGITAL</div>
                        </div>
                        <button class="btn-remove" onclick="removePin(${pin})" title="Remove pin">✕</button>
                    </div>
                    <div class="button-group">
                        <button class="btn-on" onclick="setPin(${pin}, 1)">ON</button>
                        <button class="btn-off" onclick="setPin(${pin}, 0)">OFF</button>
                    </div>
                    <button class="btn-toggle" onclick="togglePin(${pin})">Toggle</button>
                    <div class="pin-state state-low" id="state-${pin}">LOW</div>
                `;
            }
            
            grid.appendChild(card);
        }
        
        // Initialize with some common pins
        [13, 12, 14, 27].forEach(pin => createPinCard(pin, false));
        
        // Update status every 2 seconds
        updateStatus();
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
)rawliteral";
}
