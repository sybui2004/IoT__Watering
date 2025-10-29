#ifndef TELEGRAM_HANDLER_H
#define TELEGRAM_HANDLER_H

#include "config.h"

// Telegram Bot Functions
void setupTelegramBot();
void handleTelegramMessages();
void sendTelegramMessage(const String& message);
void sendSystemStatus();
void sendSensorData();
void handleTelegramCommand(const String& command, const String& chatId);

// Command handlers
void handlePumpCommand(const String& action, const String& chatId);
void handleCanopyCommand(const String& action, const String& chatId);
void handleStatusCommand(const String& chatId);
void handleHelpCommand(const String& chatId);
void handleAutoModeCommand(const String& action, const String& chatId);

// Utility functions
String getSystemStatusText();
String getSensorDataText();
String getControlStatusText();

#endif
