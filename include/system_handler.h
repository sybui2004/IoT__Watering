#ifndef SYSTEM_HANDLER_H
#define SYSTEM_HANDLER_H

#include "config.h"

void setupWatchdog();
void feedWatchdog();
void initTime();
String getISOTimestamp();
void checkDailyReset();
void printSystemStatus();

// EEPROM functions
void initEEPROM();
bool isEEPROMInitialized();
void saveDataToEEPROM();
void clearEEPROMData();
uint16_t getStoredDataCount();
void uploadStoredDataToFirebase();
void saveAutoModeToEEPROM();
bool loadAutoModeFromEEPROM();

#endif