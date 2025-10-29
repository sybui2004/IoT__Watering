#include "config.h"
#include "auto_control.h"
#include "firebase_handler.h"
#include "system_handler.h"
#include "model_final.h"

void setupAutoControl() {
    Serial.println("Khởi tạo hệ thống tự động...");
    controlData.initialized = true;
    
    // Load autoMode từ EEPROM 
    controlData.autoMode = loadAutoModeFromEEPROM();
    
    String autoStatus = controlData.autoMode ? "ON" : "OFF";
    String statusMsg = "Hệ thống tự động sẵn sàng! Chế độ tự động: ";
    statusMsg.concat(autoStatus);
    Serial.println(statusMsg);
    Serial.println("=======================");
}

void handleAutoIrrigation() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < AUTO_CONTROL_INTERVAL) return;
    lastCheck = millis();
    
    if (!controlData.autoMode) return;
    
    // Luồng hoạt động mới theo yêu cầu:
    // ESP32 nhận kết quả từ mô hình → modelPredict.needIrrigation (true/false)
    
    // Nếu modelPredict.needIrrigation = false → không tưới
    if (!modelPredict.needIrrigation) {
        if (controlData.pumpState) {
            setPumpState(false);
            uploadAlerts("irrigation", "Bơm tắt - Mô hình không khuyến nghị tưới");
        }
        return;
    }
    
    // Nếu modelPredict.needIrrigation = true → kiểm tra dự báo mưa 1 giờ tới
    // Quy tắc quyết định:
    // - Không tưới: rainNext1h > 0 mm (mưa nhỏ, vừa, to)
    // - Tưới: rainNext1h == 0 mm và cảm biến mưa hiện tại rainDetected == true
    
    bool shouldIrrigate = false;
    String reason = "";
    
    if (weatherData.rainNext1h > 0.0) {
        // Có mưa dự báo trong 1 giờ tới - không tưới
        if (controlData.pumpState) {
            setPumpState(false);
            String message = "Bơm tắt - Dự báo mưa ";
            message.concat(weatherData.rainNext1h);
            message.concat("mm trong 1h");
            uploadAlerts("irrigation", message);
        }
        shouldIrrigate = false;
    } else if (weatherData.rainNext1h == 0.0 && sensorData.rainDetected) {
        // Không mưa dự báo và không mưa hiện tại - có thể tưới
        shouldIrrigate = true;
        reason = "Mô hình khuyến nghị tưới - Không mưa";
    }
    
    // Thực hiện tưới nếu điều kiện cho phép
    if (shouldIrrigate && !controlData.pumpState) {
        setPumpState(true);
        String message = "Tự động tưới: ";
        message.concat(reason);
        uploadAlerts("irrigation", message);
        alertData.alertCountToday++;
    }
    
}

void checkAutoTurnOff() {
    // Hàm này chạy liên tục để kiểm tra tắt bơm tự động
    
    // Tự động tắt bơm sau 5 phút (300000ms)
    if (controlData.pumpState && (millis() - controlData.lastPumpOn > PUMP_INTERVAL)) {
        setPumpState(false);
        uploadAlerts("irrigation", "Bơm tắt sau 5 phút");
    }
    
    // Mái che được điều khiển bởi handleAutoCanopy() dựa trên điều kiện thời tiết
    // Không tự động tắt theo thời gian
}

void handleAutoCanopy() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < AUTO_CONTROL_INTERVAL) return;
    lastCheck = millis();
    
    if (!controlData.autoMode) return;
    
    // Luồng hoạt động mới theo yêu cầu:
    // Kiểm tra ánh sáng (BH1750) và dự báo mưa
    
    bool shouldCloseCanopy = false;
    String reason = "";
    
    // Điều kiện 1: Ánh sáng mạnh (nắng gắt)
    if (sensorData.lightLevel > LUX_HIGH_THRESHOLD) {
        shouldCloseCanopy = true;
        reason = "Ánh sáng mạnh (";
        reason.concat(sensorData.lightLevel);
        reason.concat(" lux)");
    }
    
    // Điều kiện 2: Dựa vào mưa hiện tại và mưa 1 giờ tới
    // Bật mái che (đóng): Hiện tại mưa to và rainNext1h ≥ 5 mm (mưa vừa → to)
    // Mở mái che: Hiện tại và rainNext1h < 5 mm và không mưa hiện tại
    
    if (sensorData.rainDetected && weatherData.rainNext1h >= 5.0) {
        // Hiện tại mưa và dự báo mưa vừa đến to trong 1h tới
        shouldCloseCanopy = true;
        reason = "Mưa hiện tại + dự báo mưa ";
        reason.concat(weatherData.rainNext1h);
        reason.concat("mm trong 1h");
    } else if (sensorData.rainDetected && weatherData.rainNext1h < 5.0) {
        // Không mưa hiện tại và dự báo mưa nhẹ trong 1h tới
        shouldCloseCanopy = false;
    }
    
    // Thực hiện điều khiển mái che
    if (shouldCloseCanopy && !controlData.canopyState) {
        setCanopyState(true);
        String message = "Mái che bật: ";
        message.concat(reason);
        uploadAlerts("canopy", message);
        alertData.alertCountToday++;
    } else if (!shouldCloseCanopy && controlData.canopyState) {
        setCanopyState(false);
        uploadAlerts("canopy", "Mái che tắt");
    }
    
}

// Cảnh báo thời tiết khắc nghiệt
void checkWeatherConditions() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < WEATHER_UPDATE_INTERVAL) return; 
    lastCheck = millis();
    
    
    if (sensorData.temperature > TEMP_HIGH_THRESHOLD) {
        String message = "Cảnh báo: Nhiệt độ rất cao (";
        message.concat(sensorData.temperature);
        message.concat("°C)");
        uploadAlerts("weather", message);
    }
    
    if (sensorData.temperature < TEMP_LOW_THRESHOLD) {
        String message = "Cảnh báo: Nhiệt độ rất thấp (";
        message.concat(sensorData.temperature);
        message.concat("°C)");
        uploadAlerts("weather", message);
    }

    if (sensorData.humidity > HUMIDITY_HIGH_THRESHOLD) {
        String message = "Cảnh báo: Độ ẩm rất cao (";
        message.concat(sensorData.humidity);
        message.concat("%)");
        uploadAlerts("weather", message);
    }

    if (sensorData.humidity < HUMIDITY_LOW_THRESHOLD) {
        String message = "Cảnh báo: Độ ẩm rất thấp (";
        message.concat(sensorData.humidity);
        message.concat("%)");
        uploadAlerts("weather", message);
    }
    if (sensorData.lightLevel > LUX_HIGH_THRESHOLD) {
        String message = "Cảnh báo: Ánh sáng mạnh (";
        message.concat(sensorData.lightLevel);
        message.concat(" lux)");
        uploadAlerts("weather", message);
    }
    if (sensorData.lightLevel < LUX_LOW_THRESHOLD) {
        String message = "Cảnh báo: Ánh sáng yếu (";
        message.concat(sensorData.lightLevel);
        message.concat(" lux)");
        uploadAlerts("weather", message);
    }
}


void setPumpState(bool state) {
    if (state != controlData.pumpState) {
        digitalWrite(PUMP_PIN, state ? PUMP_ON : PUMP_OFF);
        controlData.pumpState = state;
        controlData.lastPumpOn = millis();
        Serial.printf("Bơm: %s\n", state ? "ON" : "OFF");
        
        if (firebaseConnected) {
            uploadControlStatus();
        }
    }
}

void setCanopyState(bool state) {
    if (state != controlData.canopyState) {
        digitalWrite(CANOPY_PIN, state ? CANOPY_ON : CANOPY_OFF);
        controlData.canopyState = state;
        controlData.lastCanopyOn = millis();
        Serial.printf("Mái che: %s\n", state ? "ON" : "OFF");
        
        if (firebaseConnected) {
            uploadControlStatus();
        }
    }
}
