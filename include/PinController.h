#ifndef PIN_CONTROLLER_H
#define PIN_CONTROLLER_H

#include <Arduino.h>
#include <map>
#include "Config.h"

/**
 * PinController - Manages GPIO pin states and operations
 *
 * Features:
 * - Digital pin control (HIGH/LOW)
 * - PWM support for compatible pins
 * - Pin state tracking and validation
 * - Safe pin configuration
 * - State persistence support
 */

enum class PinMode
{
    DIGITAL_OUTPUT,
    PWM_OUTPUT,
    DIGITAL_INPUT,
    NOT_CONFIGURED
};

struct PinState
{
    PinMode mode;
    int value; // Digital: 0/1, PWM: 0-255
    bool isInitialized;

    PinState() : mode(PinMode::NOT_CONFIGURED), value(0), isInitialized(false) {}
};

class PinController
{
public:
    PinController();

    // Initialize the controller
    void begin();

    // Set pin to digital HIGH (1) or LOW (0)
    bool setDigital(int pin, int value);

    // Get current digital pin state
    int getDigital(int pin);

    // Toggle digital pin state
    bool toggle(int pin);

    // Set PWM value (0-255) on pin
    bool setPWM(int pin, int value);

    // Get current PWM value
    int getPWM(int pin);

    // Get pin mode
    PinMode getPinMode(int pin);

    // Check if pin is valid for control
    bool isValidPin(int pin);

    // Check if pin is configured
    bool isPinConfigured(int pin);

    // Get all configured pins and their states
    String getAllPinStates();

    // Reset all pins to default state
    void resetAllPins();

    // Get state as JSON string
    String getStateJSON();

private:
    // Configure pin for digital output
    bool configureDigitalOutput(int pin);

    // Configure pin for PWM output
    bool configurePWMOutput(int pin);

    // Check if pin supports PWM
    bool supportsPWM(int pin);

    // Pin state storage
    std::map<int, PinState> _pinStates;

    // PWM channel management (ESP32 has 16 PWM channels)
    int _nextPWMChannel;
    std::map<int, int> _pinToPWMChannel;

    static const int PWM_FREQUENCY = 5000;
    static const int PWM_RESOLUTION = 8; // 8-bit (0-255)
};

#endif // PIN_CONTROLLER_H
