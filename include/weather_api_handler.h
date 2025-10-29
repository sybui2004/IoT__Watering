#ifndef WEATHER_API_HANDLER_H
#define WEATHER_API_HANDLER_H

#include "config.h"

// Hàm để cập nhật dữ liệu thời tiết từ OpenWeatherMap API
void updateWeatherData();

// Hàm để cập nhật dự đoán từ mô hình AI
void updateModelPrediction();

// Hàm test với các scenario khác nhau
void testWeatherScenarios();

#endif


