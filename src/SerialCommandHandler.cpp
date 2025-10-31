/**
 * SerialCommandHandler.cpp
 *
 * Implementation of serial command handling
 */

#include "SerialCommandHandler.h"
#include "Config.h"

SerialCommandHandler::SerialCommandHandler(CommandParser &parser,
                                           PinController &pinCtrl,
                                           WiFiManager &wifiMgr,
                                           WatchdogManager &wdMgr)
    : _commandParser(parser),
      _pinController(pinCtrl),
      _wifiManager(wifiMgr),
      _watchdogManager(wdMgr)
{
}

void SerialCommandHandler::setRestartCallback(void (*callback)(unsigned long delayMs))
{
    _restartCallback = callback;
}

void SerialCommandHandler::processSerialCommands()
{
    if (!Serial.available())
    {
        return;
    }

    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0)
    {
        return;
    }

#if ENABLE_SERIAL_DEBUG
    Serial.printf("[Serial] Command: %s\n", command.c_str());
#endif

    // Parse command
    Command cmd = _commandParser.parse(command);

    if (!cmd.isValid())
    {
        Serial.println(_commandParser.generateResponse(cmd, false));
        return;
    }

    // Execute command
    executeCommand(cmd);
}

void SerialCommandHandler::executeCommand(const Command &cmd)
{
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
    {
        Serial.println();
        Serial.println("========================================");
        Serial.println("  System Status");
        Serial.println("========================================");
        Serial.println(_wifiManager.getStatusString());
        Serial.println();
        Serial.println("Pin States:");
        Serial.println(_pinController.getAllPinStates());
        Serial.println();
        Serial.println("Watchdog Status:");
        Serial.println(_watchdogManager.getErrorStats());
        Serial.println("========================================");
        Serial.println();
        return;
    }

    case CommandType::RESET:
        Serial.println();
        Serial.println("========================================");
        Serial.println("  RESTART REQUESTED");
        Serial.println("========================================");
        Serial.println("Restarting in 2 seconds...");
        Serial.println("========================================");
        Serial.println();

        if (_restartCallback)
        {
            _restartCallback(2000);
        }
        return;

    case CommandType::HELP:
        Serial.println();
        Serial.println("========================================");
        Serial.println("  Command Help");
        Serial.println("========================================");
        Serial.println(_commandParser.getHelpText());
        Serial.println("========================================");
        Serial.println();
        return;

    default:
        message = "Unknown command";
        success = false;
        break;
    }

    // Print response
    Serial.println(_commandParser.generateResponse(cmd, success, message, resultValue));
}
