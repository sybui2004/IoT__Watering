#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include "config.h"

void setupFirebase();
void uploadSensorData();
void uploadSystemStatus();
void uploadAlerts(String alertType, String message);
void uploadControlStatus();
void processData(AsyncResult &aResult);
void processControlCommands(AsyncResult &aResult);
void startControlStream();

#endif