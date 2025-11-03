#include "PinController.h"
#include <ArduinoJson.h>

PinController::PinController() : _nextPWMChannel(0)
{
}

void PinController::begin()
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("[PinCtrl] Pin Controller initialized");
    Serial.printf("[PinCtrl] %d safe pins available\n", SAFE_PIN_COUNT);
#endif
}

bool PinController::setDigital(int pin, int value)
{
    if (!isValidPin(pin))
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[PinCtrl] Invalid pin: %d\n", pin);
#endif
        return false;
    }

    if (value != 0 && value != 1)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[PinCtrl] Invalid digital value: %d (must be 0 or 1)\n", value);
#endif
        return false;
    }

    // Configure pin if not already configured for digital output
    PinState &state = _pinStates[pin];
    if (!state.isInitialized || state.mode != PinMode::DIGITAL_OUTPUT)
    {
        if (!configureDigitalOutput(pin))
        {
            return false;
        }
    }

    // Set the pin value
    digitalWrite(pin, value);
    state.value = value;

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[PinCtrl] Set pin %d to %d\n", pin, value);
#endif

    return true;
}

int PinController::getDigital(int pin)
{
    if (!isValidPin(pin))
    {
        return -1;
    }

    // If pin is configured, return stored state
    if (_pinStates.find(pin) != _pinStates.end())
    {
        PinState &state = _pinStates[pin];
        if (state.isInitialized && state.mode == PinMode::DIGITAL_OUTPUT)
        {
            return state.value;
        }
    }

    // Otherwise, configure as input and read current value
    pinMode(pin, INPUT);
    return digitalRead(pin);
}

bool PinController::toggle(int pin)
{
    if (!isValidPin(pin))
    {
        return false;
    }

    int currentValue = getDigital(pin);
    if (currentValue < 0)
    {
        return false;
    }

    return setDigital(pin, currentValue == 0 ? 1 : 0);
}

bool PinController::setPWM(int pin, int value)
{
    if (!isValidPin(pin))
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[PinCtrl] Invalid pin: %d\n", pin);
#endif
        return false;
    }

    if (value < 0 || value > 255)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.printf("[PinCtrl] Invalid PWM value: %d (must be 0-255)\n", value);
#endif
        return false;
    }

    // Configure pin for PWM if needed
    PinState &state = _pinStates[pin];
    if (!state.isInitialized || state.mode != PinMode::PWM_OUTPUT)
    {
        if (!configurePWMOutput(pin))
        {
            return false;
        }
    }

    // Set PWM duty cycle
    int channel = _pinToPWMChannel[pin];
    ledcWrite(channel, value);
    state.value = value;

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[PinCtrl] Set PWM on pin %d to %d (channel %d)\n", pin, value, channel);
#endif

    return true;
}

int PinController::getPWM(int pin)
{
    if (!isValidPin(pin))
    {
        return -1;
    }

    if (_pinStates.find(pin) != _pinStates.end())
    {
        PinState &state = _pinStates[pin];
        if (state.isInitialized && state.mode == PinMode::PWM_OUTPUT)
        {
            return state.value;
        }
    }

    return -1;
}

PinMode PinController::getPinMode(int pin)
{
    if (_pinStates.find(pin) != _pinStates.end())
    {
        return _pinStates[pin].mode;
    }
    return PinMode::NOT_CONFIGURED;
}

bool PinController::isValidPin(int pin)
{
    for (int i = 0; i < SAFE_PIN_COUNT; i++)
    {
        if (SAFE_PINS[i] == pin)
        {
            return true;
        }
    }
    return false;
}

bool PinController::isPinConfigured(int pin)
{
    if (_pinStates.find(pin) != _pinStates.end())
    {
        return _pinStates[pin].isInitialized;
    }
    return false;
}

String PinController::getAllPinStates()
{
    String result = "Configured Pins:\n";

    if (_pinStates.empty())
    {
        result += "  None\n";
        return result;
    }

    for (auto &pair : _pinStates)
    {
        int pin = pair.first;
        PinState &state = pair.second;

        if (state.isInitialized)
        {
            result += "  Pin " + String(pin) + ": ";

            switch (state.mode)
            {
            case PinMode::DIGITAL_OUTPUT:
                result += "DIGITAL = " + String(state.value);
                break;
            case PinMode::PWM_OUTPUT:
                result += "PWM = " + String(state.value);
                break;
            default:
                result += "UNKNOWN";
                break;
            }

            result += "\n";
        }
    }

    return result;
}

bool PinController::resetAllPins()
{
#if ENABLE_SERIAL_DEBUG
    Serial.println("[PinCtrl] Resetting all pins");
#endif

    // Set all configured pins to LOW
    for (auto &pair : _pinStates)
    {
        int pin = pair.first;
        PinState &state = pair.second;

        if (state.isInitialized)
        {
            if (state.mode == PinMode::DIGITAL_OUTPUT)
            {
                digitalWrite(pin, LOW);
            }
            else if (state.mode == PinMode::PWM_OUTPUT)
            {
                int channel = _pinToPWMChannel[pin];
                ledcWrite(channel, 0);
            }
        }
    }

    // Clear state
    _pinStates.clear();
    _pinToPWMChannel.clear();
    _nextPWMChannel = 0;

    return true;
}

String PinController::getStateJSON()
{
    JsonDocument doc;
    JsonArray pins = doc["pins"].to<JsonArray>();

    for (auto &pair : _pinStates)
    {
        int pin = pair.first;
        PinState &state = pair.second;

        if (state.isInitialized)
        {
            JsonObject pinObj = pins.add<JsonObject>();
            pinObj["pin"] = pin;
            pinObj["value"] = state.value;

            if (state.mode == PinMode::DIGITAL_OUTPUT)
            {
                pinObj["mode"] = "digital";
            }
            else if (state.mode == PinMode::PWM_OUTPUT)
            {
                pinObj["mode"] = "pwm";
            }
        }
    }

    String result;
    serializeJson(doc, result);
    return result;
}

bool PinController::configureDigitalOutput(int pin)
{
#if ENABLE_SERIAL_DEBUG
    Serial.printf("[PinCtrl] Configuring pin %d for digital output\n", pin);
#endif

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    PinState &state = _pinStates[pin];
    state.mode = PinMode::DIGITAL_OUTPUT;
    state.value = 0;
    state.isInitialized = true;

    return true;
}

bool PinController::configurePWMOutput(int pin)
{
    if (_nextPWMChannel >= 16)
    {
#if ENABLE_SERIAL_DEBUG
        Serial.println("[PinCtrl] No PWM channels available");
#endif
        return false;
    }

    int channel = _nextPWMChannel++;

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[PinCtrl] Configuring pin %d for PWM output (channel %d)\n", pin, channel);
#endif

    // Configure LED PWM channel
    ledcSetup(channel, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(pin, channel);
    ledcWrite(channel, 0);

    PinState &state = _pinStates[pin];
    state.mode = PinMode::PWM_OUTPUT;
    state.value = 0;
    state.isInitialized = true;

    _pinToPWMChannel[pin] = channel;

    return true;
}

bool PinController::supportsPWM(int pin)
{
    // On ESP32, all GPIO pins can be used for PWM (LEDC)
    // We just need to check if it's a valid output pin
    return isValidPin(pin);
}
