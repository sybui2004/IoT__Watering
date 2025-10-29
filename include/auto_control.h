#ifndef AUTO_CONTROL_H
#define AUTO_CONTROL_H

#include "config.h"
#include "model_final.h"

void setupAutoControl();
void handleAutoIrrigation();
void handleAutoCanopy();
void checkWeatherConditions();
void checkAutoTurnOff();  
void setPumpState(bool state);
void setCanopyState(bool state);

#endif
