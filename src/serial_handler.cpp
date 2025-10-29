#include "config.h"
#include "serial_handler.h"
#include "system_handler.h"     
#include "firebase_handler.h" 
#include "wifi_handler.h"
#include "auto_control.h"

void handleSerialCommands(){
    
    if (Serial.available()){
        char cmd = Serial.read();

        switch(cmd){
            case 'h': //Help
                Serial.println("Các lệnh có sẵn:");
                Serial.println("h - Help");
                Serial.println("s - Trạng thái hệ thống");
                Serial.println("p - Bật/Tắt bơm");
                Serial.println("f - Bật/Tắt mái che");
                Serial.println("a - Bật/Tắt chế độ tự động");
                Serial.println("u - Tải lên tất cả dữ liệu hiện tại");
                Serial.println("w - Kết nối WiFi");
                Serial.println("r - Kết nối Firebase");
                Serial.println("z - Test cảm biến");
                Serial.println("t - Dự báo thời tiết");
                Serial.println("=======================");
                break;
            
            case 's': //Status
                printSystemStatus();
                break;

            case 'p': //Toggle pump
                setPumpState(!controlData.pumpState);
                Serial.printf("Bơm: %s\n", controlData.pumpState ? "ON" : "OFF");
                break;
            
            case 'f': //Toggle canopy
                setCanopyState(!controlData.canopyState);
                Serial.printf("Mái che: %s\n", controlData.canopyState ? "ON" : "OFF");
                break;
            
            case 'a': //Toggle auto mode
                controlData.autoMode = !controlData.autoMode;
                Serial.printf("Chế độ tự động: %s\n", controlData.autoMode ? "ON" : "OFF");
                saveAutoModeToEEPROM(); // Save to EEPROM
                if (firebaseConnected) {
                    uploadControlStatus();
                }
                break;
            
            case 'u': //Upload all
                if (firebaseConnected){
                    uploadSensorData();
                    uploadControlStatus();
                    uploadSystemStatus();
                    Serial.println("Tất cả dữ liệu đã được tải lên Firebase");
                } else {
                    Serial.println("Firebase không kết nối. Kết nối lại với lệnh 'r'");
                }
                Serial.println("=======================");
                break;
            case 'w': // WiFi reconnect
                WiFi.disconnect();
                delay(1000);
                setupWiFi();
                break;
            
            case 'r': // Firebase reconnect
                if (WiFi.status() == WL_CONNECTED){
                    Serial.println("Đang kết nối lại Firebase...");
                    setupFirebase();
                } else {
                    Serial.println("WiFi không kết nối. Kết nối WiFi lại với lệnh 'w'");
                    Serial.println("=======================");
                }
                break;

            case 'z': // Test sensors
                Serial.println("\n=== Kiểm tra cảm biến ===");
                Serial.printf("Temperature: %.2f°C\n", sensorData.temperature);
                Serial.printf("Humidity: %.2f%%\n", sensorData.humidity);
                Serial.printf("Soil Moisture: %d\n", sensorData.soilMoisture);
                Serial.printf("Light: %.2f lux\n", sensorData.lightLevel);
                Serial.printf("Rain: %s\n", sensorData.rainDetected ? "Detected" : "Not detected");
                Serial.println("=======================");
                break;
                
            case 't': // Weather forecast
                Serial.println("\n=== Dự báo thời tiết ===");
                if (weatherData.initialized) {
                    Serial.printf("Mưa 1 giờ tới: %.2f mm\n", weatherData.rainNext1h);
                    Serial.printf("Xác suất mưa: %.0f%%\n", weatherData.popNext1h);
                    
                    // Phân tích điều kiện
                    if (weatherData.rainNext1h > 0.0) {
                        String response = "";
                        if (weatherData.rainNext1h >= 5.0) {
                            response = "🌩️ ";
                            Serial.println("🌩️ Dự báo mưa to - Mái che nên đóng");
                        } else {
                            response = "🌧️ ";
                            Serial.println("🌧️ Dự báo mưa vừa - Mái che có thể mở");
                        }
                        response.concat("Dự báo mưa - Không nên tưới");
                        Serial.println(response);
                    } else {
                        Serial.println("☀️ Dự báo không mưa - Có thể tưới nếu mô hình đề xuất tưới");
                    }
                    
                    // Hiển thị thời gian cập nhật cuối
                    unsigned long timeSinceUpdate = millis() - weatherData.lastUpdate;
                    Serial.printf("Cập nhật cuối: %lu giây trước\n", timeSinceUpdate / 1000);
                } else {
                    Serial.println("Lỗi dữ liệu");
                    Serial.println("Kiểm tra kết nối WiFi và API thời tiết");
                }
                Serial.println("========================");
                break;
                
                
        }

        // Clear any remaining characters
        while (Serial.available()){
            Serial.read();
        }
    }
}