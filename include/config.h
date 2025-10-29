// config.h

#ifndef CONFIG_H
#define CONFIG_H

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <FirebaseJSON.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <time.h>
#include <EEPROM.h>
#include "esp_system.h"
#include "esp_task_wdt.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Các chân pin
#define DHTPIN 4
#define DHTTYPE DHT11
#define SOIL_PIN 34
#define RAIN_PIN 33
#define CANOPY_PIN 26
#define PUMP_PIN 27
#define CANOPY_ON LOW
#define CANOPY_OFF HIGH
#define PUMP_ON LOW
#define PUMP_OFF HIGH

// Các giá trị EEPROM
#define EEPROM_SIZE 4096
// header
// Địa chỉ lưu magic number để đánh dấu EEPROM đã được khởi tạo
// Nếu giá trị này != EEPROM_MAGIC_NUMBER chứng tỏ lần đầu chạy
#define EEPROM_MAGIC_ADDR 0
// Địa chỉ lưu số lượng bản ghi đã lưu trong EEPROM
#define EEPROM_DATA_COUNT_ADDR 2
// Tổng kích thước phần header 
#define EEPROM_HEADER_SIZE 16 
// Kích thước mỗi bản ghi 
#define EEPROM_RECORD_SIZE 32
// Số bản ghi tối đa 32*100 = 3200
#define EEPROM_MAX_RECORDS 100
// Địa chỉ bắt đầu vùng lưu dữ liệu (sau header)
#define EEPROM_DATA_START_ADDR EEPROM_HEADER_SIZE 
// Địa chỉ kết thúc vùng dữ liệu 
#define EEPROM_DATA_END_ADDR (EEPROM_DATA_START_ADDR + EEPROM_RECORD_SIZE * EEPROM_MAX_RECORDS)

#define EEPROM_AUTOMODE_ADDR 3500
#define EEPROM_MAGIC_NUMBER 0x1AA1 // 2 bytes
#define EEPROM_SAVE_INTERVAL 300000 // 5 minutes

// Thời gian watchdog timeout
#define WDT_TIMEOUT 30

// Thông tin WiFi 
#define WIFI_SSID "Sy1"
#define WIFI_PASSWORD "12345678"

// Thông tin Firebase
#define WEB_API_KEY ""
#define DATABASE_URL ""
#define USER_EMAIL ""
#define USER_PASSWORD ""
#define ROOT "IrrigationSystem/ESP1"

// Thông tin Telegram Bot
#define TELEGRAM_BOT_TOKEN ""
#define TELEGRAM_UPDATE_INTERVAL 2000 // 2 seconds
// Khai báo mảng và biến đếm — chỉ khai báo, chưa cấp bộ nhớ
extern const String TELEGRAM_CHAT_IDS[];
extern const int TELEGRAM_CHAT_COUNT;

// Thông tin OpenWeather 
#define OPENWEATHER_HOST "https://pro.openweathermap.org"
#define OPENWEATHER_LAT "20.980913"
#define OPENWEATHER_LON "105.7874165"
#define OPENWEATHER_UNITS "metric"
#define OPENWEATHER_CNT "1"
#define API_KEY_WEATHER_API ""

// Thông tin NTP Server
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 25200 // GMT+7
#define DAYLIGHT_OFFSET_SEC 0

// Các giá trị ngưỡng sensor
#define SOIL_DRY_THRESHOLD 2000
#define SOIL_WET_THRESHOLD 1000
#define TEMP_HIGH_THRESHOLD 30.0
#define TEMP_LOW_THRESHOLD 13.0
#define HUMIDITY_LOW_THRESHOLD 30.0
#define HUMIDITY_HIGH_THRESHOLD 85.0
#define LUX_LOW_THRESHOLD 10000
#define LUX_HIGH_THRESHOLD 20000

// Các thời gian interval
#define SENSOR_READ_INTERVAL 5000
#define FIREBASE_UPLOAD_INTERVAL 300000 // 5 minutes (Lúc demo để 15 giây = 15000)
#define AUTO_CONTROL_INTERVAL 600000 // 10 minutes (Lúc demo để 30 giây = 30000)
#define STATUS_UPDATE_INTERVAL 300000 // 5 minutes (Lúc demo để 15 giây = 15000)
#define HEALTH_CHECK_INTERVAL 600000 // 10 minutes (Lúc demo để 30 giây = 30000)
#define PUMP_INTERVAL 300000 // 5 minutes (Lúc demo để 10 giây = 10000)
#define WEATHER_UPDATE_INTERVAL 600000 // 10 minutes (Lúc demo để 30 giây = 30000)
#define WIFI_RECONNECT_INTERVAL 300000 // 5 minutes 

// Các biến toàn cục
extern FirebaseApp app;
extern WiFiClientSecure fb_ssl_client; // Firebase TLS client
extern WiFiClientSecure tg_ssl_client; // Telegram TLS client
extern WiFiClientSecure http_ssl_client; // HTTP/Weather TLS client
using AsyncClient = AsyncClientClass;
extern AsyncClient aClient;
extern RealtimeDatabase Database;
extern AsyncResult dbResult;
extern UserAuth user_auth;
extern BH1750 lightMeter;
extern DHT dht;
extern UniversalTelegramBot* telegramBot;
extern bool firebaseConnected;

// Các struct
struct SensorData {
    float temperature = 0;
    float humidity = 0;
    int soilMoisture = 0;
    float lightLevel = 0;
    bool rainDetected = false;
    unsigned long lastRead = 0;
    unsigned long lastUpload = 0;
    bool initialized = false;
    bool error = false;
};
extern SensorData sensorData;

struct ControlData {
    bool pumpState = false;
    bool canopyState = false;
    bool autoMode = true;
    unsigned long lastPumpOn = 0;
    unsigned long lastCanopyOn = 0;
    bool initialized = false;
};
extern ControlData controlData;

struct SystemState {
    time_t lastResetTime = 0;
    struct tm timeinfo;
    unsigned long uptimeSeconds = 0;
    unsigned long lastUptimeUpdate = 0;
    unsigned long lastCheckHealth = 0;
    bool timeInitialized = false;
    bool eepromInitialized = false;
};
extern SystemState systemState;

struct AlertData {
    unsigned long lastAlertTime = 0;
    int alertCountToday = 0;
    bool initialized = false;
};
extern AlertData alertData;

struct ModelPredict {
    bool needIrrigation = false;
    unsigned long lastUpdate = 0;
    bool initialized = false;
};
extern ModelPredict modelPredict;

struct WeatherData {
    float rainNext1h = 0.0;        // Lượng mưa dự báo trong 1 giờ tới (mm)
    float popNext1h = 0.0;         // Xác suất mưa trong 1 giờ tới (%)
    unsigned long lastUpdate = 0;
    bool initialized = false;
};
extern WeatherData weatherData;

// 32 bytes
struct StoredData {
    unsigned long timestamp; // 4 bytes
    float temperature; // 4 bytes
    float humidity; // 4 bytes
    int soilMoisture; // 4 bytes
    float lightLevel; // 4 bytes
    bool rainDetected; // 1 byte
    bool pumpState; // 1 byte
    bool canopyState; // 1 byte
    bool autoMode; // 1 byte
    char tmp[8]; // 8 bytes
};

#endif