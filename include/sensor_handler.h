#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H

#include "config.h"

void setupSensors();
void readSensors();
void readDHT11();
void readSoilMoisture();
void readRainSensor();
void readLightSensor();

#endif
