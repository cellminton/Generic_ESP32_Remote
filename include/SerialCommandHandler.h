/**
 * SerialCommandHandler.h
 *
 * Handles serial port commands for debugging and testing
 */

#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>
#include "CommandParser.h"
#include "PinController.h"
#include "WiFiManager.h"
#include "WatchdogManager.h"

class SerialCommandHandler
{
public:
    SerialCommandHandler(CommandParser &parser,
                         PinController &pinCtrl,
                         WiFiManager &wifiMgr,
                         WatchdogManager &wdMgr);

    // Process available serial commands
    void processSerialCommands();

    // Set restart callback (called when RESET command is received)
    void setRestartCallback(void (*callback)(unsigned long delayMs));

private:
    CommandParser &_commandParser;
    PinController &_pinController;
    WiFiManager &_wifiManager;
    WatchdogManager &_watchdogManager;

    void (*_restartCallback)(unsigned long) = nullptr;

    // Execute a parsed command
    void executeCommand(const Command &cmd);
};

#endif // SERIAL_COMMAND_HANDLER_H
