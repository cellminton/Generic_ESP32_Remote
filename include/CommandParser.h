#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"

/**
 * Command Parser - Handles parsing and validation of pin control commands
 *
 * Supported command formats:
 *
 * JSON Format:
 * {"cmd":"SET","pin":13,"value":1}
 * {"cmd":"GET","pin":13}
 * {"cmd":"TOGGLE","pin":13}
 * {"cmd":"PWM","pin":13,"value":128}
 * {"cmd":"STATUS"}
 * {"cmd":"RESET"}
 *
 * Text Format:
 * SET 13 1
 * GET 13
 * TOGGLE 13
 * PWM 13 128
 * STATUS
 * RESET
 */

enum class CommandType
{
    INVALID,
    SET,        // Set pin to HIGH or LOW
    GET,        // Get current pin state
    TOGGLE,     // Toggle pin state
    PWM,        // Set PWM value (0-255)
    STATUS,     // Get system status
    RESET,      // Reset/restart system
    RESET_PINS, // Reset all pins to LOW
    HELP        // Get help information
};

struct Command
{
    CommandType type;
    int pin;
    int value;
    String errorMessage;

    Command() : type(CommandType::INVALID), pin(-1), value(-1), errorMessage("") {}

    bool isValid() const
    {
        return type != CommandType::INVALID;
    }
};

class CommandParser
{
public:
    CommandParser();

    // Parse command from string (auto-detects JSON or text format)
    Command parse(const String &commandString);

    // Generate response for a command
    String generateResponse(const Command &cmd, bool success,
                            const String &message = "", int resultValue = -1);

    // Get help text
    String getHelpText();

private:
    // Parse JSON format command
    Command parseJSON(const String &jsonString);

    // Parse text format command
    Command parseText(const String &textString);

    // Validate pin number
    bool isValidPin(int pin);

    // Convert string to command type
    CommandType stringToCommandType(const String &cmdStr);

    // Convert command type to string
    String commandTypeToString(CommandType type);
};

#endif // COMMAND_PARSER_H
