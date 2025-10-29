#include "config.h"
#include "wifi_handler.h"
#include "system_handler.h"

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true); 
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Kết nối WiFi...");
    
    // Try to connect for maximum 30 seconds to avoid blocking
    unsigned long startTime = millis();
    int dotCount = 0;
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 30000) {
        delay(500);
        dotCount++;
        if (dotCount % 6 == 0) {  // Print every 3 seconds
            Serial.print(".");
            Serial.flush();
        }
        feedWatchdog();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("WiFi kết nối thành công!");
        Serial.print("Địa chỉ IP: ");
        Serial.println(WiFi.localIP());
        
        Serial.println("Đang cấu hình SSL client...");
        fb_ssl_client.setInsecure();
        fb_ssl_client.setTimeout(10000); 
        fb_ssl_client.setHandshakeTimeout(10);  
        // Cấu hình client riêng cho Telegram
        tg_ssl_client.setInsecure();
        tg_ssl_client.setTimeout(10000);
        tg_ssl_client.setHandshakeTimeout(10);
        // Cấu hình client cho Weather/HTTP
        http_ssl_client.setInsecure();
        http_ssl_client.setTimeout(10000);
        http_ssl_client.setHandshakeTimeout(10);
        Serial.println("SSL client đã được cấu hình");
    } else {
        Serial.println();
        Serial.println("WiFi không kết nối. SSL client chưa được cấu hình.");
        Serial.println("Sẽ thử kết nối WiFi lại sau.");
    }
}

void checkWiFiConnection() {
    static unsigned long lastCheck = 0;
    static unsigned long lastReconnectAttempt = 0;
    
    if (millis() - lastCheck > WIFI_RECONNECT_INTERVAL) {
        if (WiFi.status() != WL_CONNECTED) {
            // Only attempt reconnect every 30 seconds to avoid spam
            if (millis() - lastReconnectAttempt > 30000) {
                Serial.println("WiFi không kết nối, đang thử kết nối...");
                WiFi.disconnect();
                setupWiFi();
                
                // Nếu WiFi kết nối thành công sau khi reconnect, cấu hình lại SSL
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("WiFi đã kết nối lại, cấu hình lại SSL client...");
                    fb_ssl_client.setInsecure();
                    fb_ssl_client.setTimeout(10000); 
                    fb_ssl_client.setHandshakeTimeout(10);
                    tg_ssl_client.setInsecure();
                    tg_ssl_client.setTimeout(10000);
                    tg_ssl_client.setHandshakeTimeout(10);
                    http_ssl_client.setInsecure();
                    http_ssl_client.setTimeout(10000);
                    http_ssl_client.setHandshakeTimeout(10);
                    Serial.println("SSL client đã được cấu hình lại");
                }
                
                lastReconnectAttempt = millis();
            }
        }
        lastCheck = millis();
    }
}