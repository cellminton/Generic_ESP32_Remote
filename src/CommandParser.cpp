#include "CommandParser.h"

CommandParser::CommandParser()
{
}

Command CommandParser::parse(const String &commandString)
{
    Command cmd;

    if (commandString.length() == 0)
    {
        cmd.errorMessage = "Empty command";
        return cmd;
    }

    // Trim whitespace
    String trimmed = commandString;
    trimmed.trim();

    // Auto-detect format (JSON starts with '{')
    if (trimmed.startsWith("{"))
    {
        return parseJSON(trimmed);
    }
    else
    {
        return parseText(trimmed);
    }
}

Command CommandParser::parseJSON(const String &jsonString)
{
    Command cmd;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error)
    {
        cmd.errorMessage = "JSON parse error: " + String(error.c_str());
        return cmd;
    }

    // Get command type
    if (!doc.containsKey("cmd"))
    {
        cmd.errorMessage = "Missing 'cmd' field";
        return cmd;
    }

    String cmdStr = doc["cmd"].as<String>();
    cmdStr.toUpperCase();
    cmd.type = stringToCommandType(cmdStr);

    if (cmd.type == CommandType::INVALID)
    {
        cmd.errorMessage = "Invalid command type: " + cmdStr;
        return cmd;
    }

    // Parse parameters based on command type
    switch (cmd.type)
    {
    case CommandType::SET:
    case CommandType::GET:
    case CommandType::TOGGLE:
    case CommandType::PWM:
        if (!doc.containsKey("pin"))
        {
            cmd.errorMessage = "Missing 'pin' field";
            cmd.type = CommandType::INVALID;
            return cmd;
        }
        cmd.pin = doc["pin"];

        if (!isValidPin(cmd.pin))
        {
            cmd.errorMessage = "Invalid pin number: " + String(cmd.pin);
            cmd.type = CommandType::INVALID;
            return cmd;
        }

        // SET and PWM require value
        if (cmd.type == CommandType::SET || cmd.type == CommandType::PWM)
        {
            if (!doc.containsKey("value"))
            {
                cmd.errorMessage = "Missing 'value' field";
                cmd.type = CommandType::INVALID;
                return cmd;
            }
            cmd.value = doc["value"];

            // Validate value range
            if (cmd.type == CommandType::SET && (cmd.value != 0 && cmd.value != 1))
            {
                cmd.errorMessage = "SET value must be 0 or 1";
                cmd.type = CommandType::INVALID;
                return cmd;
            }
            if (cmd.type == CommandType::PWM && (cmd.value < 0 || cmd.value > 255))
            {
                cmd.errorMessage = "PWM value must be 0-255";
                cmd.type = CommandType::INVALID;
                return cmd;
            }
        }
        break;

    case CommandType::STATUS:
    case CommandType::RESET:
    case CommandType::RESET_PINS:
    case CommandType::HELP:
        // These commands don't require parameters
        break;

    default:
        cmd.errorMessage = "Unknown command type";
        cmd.type = CommandType::INVALID;
        break;
    }

    return cmd;
}

Command CommandParser::parseText(const String &textString)
{
    Command cmd;

    // Split command into tokens
    String text = textString;
    text.toUpperCase();

    int firstSpace = text.indexOf(' ');
    String cmdStr;
    String params;

    if (firstSpace == -1)
    {
        cmdStr = text;
        params = "";
    }
    else
    {
        cmdStr = text.substring(0, firstSpace);
        params = text.substring(firstSpace + 1);
    }

    cmdStr.trim();
    params.trim();

    cmd.type = stringToCommandType(cmdStr);

    if (cmd.type == CommandType::INVALID)
    {
        cmd.errorMessage = "Invalid command: " + cmdStr;
        return cmd;
    }

    // Parse parameters based on command type
    switch (cmd.type)
    {
    case CommandType::SET:
    case CommandType::PWM:
    {
        // Format: SET pin value  or  PWM pin value
        int spacePos = params.indexOf(' ');
        if (spacePos == -1)
        {
            cmd.errorMessage = "Missing parameters (expected: pin value)";
            cmd.type = CommandType::INVALID;
            return cmd;
        }

        String pinStr = params.substring(0, spacePos);
        String valueStr = params.substring(spacePos + 1);
        pinStr.trim();
        valueStr.trim();

        cmd.pin = pinStr.toInt();
        cmd.value = valueStr.toInt();

        if (!isValidPin(cmd.pin))
        {
            cmd.errorMessage = "Invalid pin number: " + String(cmd.pin);
            cmd.type = CommandType::INVALID;
            return cmd;
        }

        if (cmd.type == CommandType::SET && (cmd.value != 0 && cmd.value != 1))
        {
            cmd.errorMessage = "SET value must be 0 or 1";
            cmd.type = CommandType::INVALID;
            return cmd;
        }
        if (cmd.type == CommandType::PWM && (cmd.value < 0 || cmd.value > 255))
        {
            cmd.errorMessage = "PWM value must be 0-255";
            cmd.type = CommandType::INVALID;
            return cmd;
        }
        break;
    }

    case CommandType::GET:
    case CommandType::TOGGLE:
    {
        // Format: GET pin  or  TOGGLE pin
        if (params.length() == 0)
        {
            cmd.errorMessage = "Missing pin parameter";
            cmd.type = CommandType::INVALID;
            return cmd;
        }

        cmd.pin = params.toInt();

        if (!isValidPin(cmd.pin))
        {
            cmd.errorMessage = "Invalid pin number: " + String(cmd.pin);
            cmd.type = CommandType::INVALID;
            return cmd;
        }
        break;
    }

    case CommandType::STATUS:
    case CommandType::RESET:
    case CommandType::RESET_PINS:
    case CommandType::HELP:
        // No parameters needed
        break;

    default:
        cmd.errorMessage = "Unknown command";
        cmd.type = CommandType::INVALID;
        break;
    }

    return cmd;
}

String CommandParser::generateResponse(const Command &cmd, bool success,
                                       const String &message, int resultValue)
{
    JsonDocument doc;

    doc["success"] = success;
    doc["command"] = commandTypeToString(cmd.type);

    if (cmd.pin >= 0)
    {
        doc["pin"] = cmd.pin;
    }

    if (resultValue >= 0)
    {
        doc["value"] = resultValue;
    }

    if (message.length() > 0)
    {
        doc["message"] = message;
    }
    else if (!success && cmd.errorMessage.length() > 0)
    {
        doc["message"] = cmd.errorMessage;
    }

    String response;
    serializeJson(doc, response);
    return response;
}

String CommandParser::getHelpText()
{
    String help = "ESP32 Pin Controller - Command Reference\n\n";
    help += "JSON Format:\n";
    help += "  Set pin:    {\"cmd\":\"SET\",\"pin\":13,\"value\":1}\n";
    help += "  Get pin:    {\"cmd\":\"GET\",\"pin\":13}\n";
    help += "  Toggle pin: {\"cmd\":\"TOGGLE\",\"pin\":13}\n";
    help += "  PWM:        {\"cmd\":\"PWM\",\"pin\":13,\"value\":128}\n";
    help += "  Status:     {\"cmd\":\"STATUS\"}\n";
    help += "  Reset:      {\"cmd\":\"RESET\"}\n\n";
    help += "Text Format:\n";
    help += "  Set pin:    SET 13 1\n";
    help += "  Get pin:    GET 13\n";
    help += "  Toggle pin: TOGGLE 13\n";
    help += "  PWM:        PWM 13 128\n";
    help += "  Status:     STATUS\n";
    help += "  Reset:      RESET\n\n";
    help += "Available pins: ";
    for (int i = 0; i < SAFE_PIN_COUNT; i++)
    {
        if (i > 0)
            help += ", ";
        help += String(SAFE_PINS[i]);
    }
    help += "\n";
    return help;
}

bool CommandParser::isValidPin(int pin)
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

CommandType CommandParser::stringToCommandType(const String &cmdStr)
{
    if (cmdStr == "SET")
        return CommandType::SET;
    if (cmdStr == "GET")
        return CommandType::GET;
    if (cmdStr == "TOGGLE")
        return CommandType::TOGGLE;
    if (cmdStr == "PWM")
        return CommandType::PWM;
    if (cmdStr == "STATUS")
        return CommandType::STATUS;
    if (cmdStr == "RESET")
        return CommandType::RESET;
    if (cmdStr == "RESET_PINS")
        return CommandType::RESET_PINS;
    if (cmdStr == "HELP")
        return CommandType::HELP;
    return CommandType::INVALID;
}

String CommandParser::commandTypeToString(CommandType type)
{
    switch (type)
    {
    case CommandType::SET:
        return "SET";
    case CommandType::GET:
        return "GET";
    case CommandType::TOGGLE:
        return "TOGGLE";
    case CommandType::PWM:
        return "PWM";
    case CommandType::STATUS:
        return "STATUS";
    case CommandType::RESET:
        return "RESET";
    case CommandType::RESET_PINS:
        return "RESET_PINS";
    case CommandType::HELP:
        return "HELP";
    default:
        return "INVALID";
    }
}
