#include "config.h"
#include "wifi_handler.h"
#include "firebase_handler.h"
#include "system_handler.h"
#include "sensor_handler.h"
#include "auto_control.h"
#include "serial_handler.h"
#include "health_check.h"
#include "weather_api_handler.h"
#include "telegram_handler.h"

// Global Objects
FirebaseApp app;
WiFiClientSecure fb_ssl_client;
WiFiClientSecure tg_ssl_client;
WiFiClientSecure http_ssl_client;
AsyncClient aClient(fb_ssl_client);
RealtimeDatabase Database;
AsyncResult dbResult;
UserAuth user_auth(WEB_API_KEY, USER_EMAIL, USER_PASSWORD);
BH1750 lightMeter;
DHT dht(DHTPIN, DHTTYPE);

// Global Variables
bool firebaseConnected = false;

// Global Structs
SensorData sensorData;
ControlData controlData;
SystemState systemState;
AlertData alertData;
ModelPredict modelPredict;
WeatherData weatherData;

const String TELEGRAM_CHAT_IDS[] = {
    "8060435963",
    "6422459919"
};

const int TELEGRAM_CHAT_COUNT = sizeof(TELEGRAM_CHAT_IDS) / sizeof(TELEGRAM_CHAT_IDS[0]);
void setup(){
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Smart Irrigation System ===");
    StoredData data;
    Serial.printf("StoredData size: %d bytes\n", sizeof(StoredData));
    
    // Initialize EEPROM
    initEEPROM();

    // Setup Watchdog
    setupWatchdog();

    // Initialize sensors
    setupSensors();
    setupAutoControl();

    // Setup WiFi
    setupWiFi();
    
    if (WiFi.status() == WL_CONNECTED){
        initTime();
        setupFirebase();
        setupTelegramBot();
        
        // Upload stored data when reconnected
        if (getStoredDataCount() > 0) {
            Serial.println("Tìm thấy dữ liệu trong EEPROM, tải lên...");
            uploadStoredDataToFirebase();
        }
    } else {
        Serial.println("WiFi không kết nối. Firebase và Telegram Bot sẽ không hoạt động.");
        Serial.println("Sử dụng lệnh 'w' để kết nối WiFi lại.");
    }

    Serial.println("Hệ thống đã khởi tạo và sẵn sàng!");
    Serial.println("Các lệnh: h(elp), s(tatus), p(ump), f(canopy), a(auto), u(pload), w(ifi), r(firebase), z(test)");
}

void loop(){
    // Feed the watchdog
    feedWatchdog();

    // Update system uptime
    if (millis() - systemState.lastUptimeUpdate >= 60000) {  // Every minute
        systemState.uptimeSeconds += 60;
        systemState.lastUptimeUpdate = millis();
    }

    // Read all sensors
    readSensors();

    // Process Firebase
    app.loop();
    
    // Check if authentication is ready AND WiFi is connected
    if (app.ready() && WiFi.status() == WL_CONNECTED){
        if (!firebaseConnected) {
            Serial.println("Firebase đã kết nối!");
            firebaseConnected = true;
            // Start listening to realtime control commands
            startControlStream();
            // Upload stored data when reconnected
            if (getStoredDataCount() > 0) {
                Serial.println("Tìm thấy dữ liệu trong EEPROM, tải lên...");
                uploadStoredDataToFirebase();
            }
        } else {
            firebaseConnected = true;
        }
        
        // Upload sensor data every minute
        if (millis() - sensorData.lastUpload >= FIREBASE_UPLOAD_INTERVAL) {
            uploadSensorData();
            sensorData.lastUpload = millis();
        }
        
        // Check for EEPROM data to upload every 30 seconds
        static unsigned long lastEEPROMCheck = 0;
        if (millis() - lastEEPROMCheck >= 30000) {
            if (getStoredDataCount() > 0) {
                Serial.println("Tìm thấy dữ liệu trong EEPROM, tải lên...");
                uploadStoredDataToFirebase();
            }
            lastEEPROMCheck = millis();
        }

        // Update system status every 5 minutes
        if (millis() - systemState.lastCheckHealth >= STATUS_UPDATE_INTERVAL){
            uploadSystemStatus();
            systemState.lastCheckHealth = millis();
        }

        // Health check every 10 minutes
        if (millis() - systemState.lastCheckHealth >= HEALTH_CHECK_INTERVAL){
            checkSystemHealth();
            systemState.lastCheckHealth = millis();
        }
    } else {
        // Firebase not connected - save to EEPROM
        if (firebaseConnected) {
            Serial.println("Firebase không kết nối!");
        }
        firebaseConnected = false;
        static unsigned long lastSaveToEEPROM = 0;
        static unsigned long lastDebugPrint = 0;
        unsigned long timeSinceLastSave = millis() - lastSaveToEEPROM;
        
        if (timeSinceLastSave >= EEPROM_SAVE_INTERVAL) { 
            Serial.println("Firebase không kết nối - lưu vào EEPROM...");
            saveDataToEEPROM();
            lastSaveToEEPROM = millis();
        }
    }

    // Update weather data and model predictions
    updateWeatherData();
    updateModelPrediction();
    
    // Auto control system
    if (controlData.autoMode) {
        handleAutoIrrigation();        // Logic chính với XGBoost + Weather API
        handleAutoCanopy();            // Logic mái che với ánh sáng + dự báo mưa
        checkWeatherConditions();      // Cảnh báo thời tiết khắc nghiệt
    }
    
    // Kiểm tra tắt bơm tự động
    checkAutoTurnOff();

    // Check for daily reset at midnight
    checkDailyReset();

    // Print system status every 60 seconds
    static unsigned long lastPrintStatus = 0;
    if (millis() - lastPrintStatus >= 60000){
        printSystemStatus();
        lastPrintStatus = millis();
    }

    // Process any serial commands
    handleSerialCommands();

    // Handle Telegram messages
    if (WiFi.status() == WL_CONNECTED) {
        handleTelegramMessages();
    }

    // Check WiFi connection and attempt reconnection if needed
    checkWiFiConnection();

    // Flush serial output to ensure immediate response
    Serial.flush();
    
    // Very small delay to prevent CPU hogging
    delay(10);
}