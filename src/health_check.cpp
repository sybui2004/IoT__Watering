#include "health_check.h"
#include "config.h"
#include "firebase_handler.h"

void handleHealthCheck(){
    Serial.println("===== Check hệ thống =====");
    Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Soil: %d, Light: %.2f lux\n",
            sensorData.temperature,
            sensorData.humidity,
            sensorData.soilMoisture,
            sensorData.lightLevel);
    
    // Kiểm tra sức khỏe hệ thống
    bool systemHealthy = true;
    String issues = "";
    
    // Kiểm tra cảm biến
    if (sensorData.error) {
        systemHealthy = false;
        issues += "Lỗi cảm biến; ";
    }
    
    // Kiểm tra kết nối
    if (!firebaseConnected) {
        systemHealthy = false;
        issues += "Firebase không kết nối; ";
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        systemHealthy = false;
        issues += "WiFi không kết nối; ";
    }
    
    // Kiểm tra giá trị cảm biến bất thường
    if (sensorData.temperature < -10 || sensorData.temperature > 60) {
        systemHealthy = false;
        issues += "Temperature out of range; ";
    }
    
    if (sensorData.humidity < 0 || sensorData.humidity > 100) {
        systemHealthy = false;
        issues += "Humidity out of range; ";
    }
    
    if (sensorData.soilMoisture < 0 || sensorData.soilMoisture > 1023) {
        systemHealthy = false;
        issues += "Soil sensor out of range; ";
    }
    
    if (systemHealthy) {
        Serial.println("Health Status: HEALTHY");
        if (firebaseConnected) {
            uploadAlerts("health", "System healthy - all sensors OK");
        }
    } else {
        Serial.println("Trạng thái: KHÔNG TỐT");
        Serial.printf("Issues: %s\n", issues.c_str());
        String message = "Hệ thống không tốt - ";
        message.concat(issues);
        if (firebaseConnected) {
            uploadAlerts("health", message);
        }
    }
}

void checkSystemHealth(){
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < HEALTH_CHECK_INTERVAL) return;
    lastCheck = millis();
    
    handleHealthCheck();
}