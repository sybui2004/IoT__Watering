#include "config.h" 
#include "system_handler.h"     
#include "firebase_handler.h"   

void setupWatchdog() {
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL);
    Serial.println("Watchdog khởi tạo hoàn tất");
}

void feedWatchdog() {
    esp_task_wdt_reset(); 
}

void initTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    if (!getLocalTime(&systemState.timeinfo)) {
        Serial.println("Lỗi lấy thời gian!");
        return;
    }
    
    time(&systemState.lastResetTime);
    systemState.timeInitialized = true;
    
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &systemState.timeinfo);
    Serial.print("Thời gian hiện tại: ");
    Serial.println(timeStr);
}

String getISOTimestamp() {
    if (!getLocalTime(&systemState.timeinfo)) {
        return "time-error";
    }
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", &systemState.timeinfo);
    return String(timestamp);
}

void checkDailyReset() {
    if (!systemState.timeInitialized || !getLocalTime(&systemState.timeinfo)) {
        return;
    }
    
    time_t now;
    time(&now);

    if (systemState.timeinfo.tm_hour == 0 && systemState.timeinfo.tm_min == 0 && difftime(now, systemState.lastResetTime) > 3600) {
        Serial.println("Thực hiện khởi động lại hàng ngày...");   

        alertData.alertCountToday = 0;
        time(&systemState.lastResetTime);

        if (firebaseConnected) {
            FirebaseJson json;
            json.set("count_today", 0);
            json.set("active", false);
            Database.set<object_t>(aClient, "alerts/current/irrigation", object_t(json.raw()), processData, "reset irrigation status");
        }

        Serial.println("Khởi động lại hàng ngày hoàn tất.");
    }
}

void printSystemStatus(){
    Serial.println("\n=== Trạng thái hệ thống ===");
    Serial.print("Thời gian: ");
    Serial.println(getISOTimestamp());

    Serial.println("\nĐọc cảm biến:");
    Serial.printf("  Temperature: %.2f°C\n", sensorData.temperature);
    Serial.printf("  Humidity: %.2f%%\n", sensorData.humidity);
    Serial.printf("  Soil Moisture: %d\n", sensorData.soilMoisture);
    Serial.printf("  Light Level: %.2f lux\n", sensorData.lightLevel);
    Serial.printf("  Rain Detected: %s\n", sensorData.rainDetected ? "Yes" : "No");

    Serial.println("\nTrạng thái điều khiển:");
    Serial.printf("  Pump: %s\n", controlData.pumpState ? "ON" : "OFF");
    Serial.printf("  Canopy: %s\n", controlData.canopyState ? "ON" : "OFF");
    Serial.printf("  Auto Mode: %s\n", controlData.autoMode ? "Enabled" : "Disabled");

    Serial.println("\nKết nối:");
    Serial.printf("  WiFi: %s (RSSI: %ddBm)\n", WiFi.status() == WL_CONNECTED ? "Kết nối" : "Không kết nối", WiFi.RSSI());
    Serial.printf("  Firebase: %s\n", firebaseConnected ? "Kết nối" : "Không kết nối");
    
    Serial.printf("  Thời gian đã chạy: %lu phút\n", systemState.uptimeSeconds / 60);

    Serial.println("\nTrạng thái cảm biến:");
    Serial.printf("  DHT11: %s\n", sensorData.error ? "ERROR" : "OK");
    Serial.printf("  Soil Sensor: %s\n", "OK");
    Serial.printf("  Rain Sensor: %s\n", "OK");
    Serial.printf("  Light Sensor: %s\n", sensorData.lightLevel > 0 ? "OK" : "ERROR");
    Serial.printf("  EEPROM Records: %d\n", getStoredDataCount());
    Serial.println("================================");
}

// EEPROM Functions
void initEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    
    // Kiểm tra xem EEPROM đã được khởi tạo chưa
    if (!isEEPROMInitialized()) {
        Serial.println("Khởi tạo EEPROM lần đầu...");
        EEPROM.put(EEPROM_MAGIC_ADDR, (uint16_t)EEPROM_MAGIC_NUMBER);
        EEPROM.put(EEPROM_DATA_COUNT_ADDR, (uint16_t)0);
        EEPROM.commit();
        systemState.eepromInitialized = true;
        Serial.println("EEPROM khởi tạo thành công");
    } else {
        systemState.eepromInitialized = true;
        Serial.printf("EEPROM đã được khởi tạo trước đó. Số bản ghi: %d\n", getStoredDataCount());
    }
}

bool isEEPROMInitialized() {
    uint16_t magic;
    EEPROM.get(EEPROM_MAGIC_ADDR, magic);
    return (magic == EEPROM_MAGIC_NUMBER);
}

void saveDataToEEPROM() {
    if (!systemState.eepromInitialized) {
        Serial.println("EEPROM chưa được khởi tạo!");
        return;
    }
    
    uint16_t count = getStoredDataCount();
    if (count >= EEPROM_MAX_RECORDS) {
        Serial.println("EEPROM đầy! Ghi đè từ đầu....");
        count = 0;
    }
    
    StoredData data;
    // Lưu timestamp thực tế từ NTP thay vì millis()
    if (systemState.timeInitialized && getLocalTime(&systemState.timeinfo)) {
        data.timestamp = mktime(&systemState.timeinfo);
    } else {
        data.timestamp = millis(); // Fallback nếu không có NTP
    }
    data.temperature = sensorData.temperature;
    data.humidity = sensorData.humidity;
    data.soilMoisture = sensorData.soilMoisture;
    data.lightLevel = sensorData.lightLevel;
    data.rainDetected = sensorData.rainDetected;
    data.pumpState = controlData.pumpState;
    data.canopyState = controlData.canopyState;
    data.autoMode = controlData.autoMode;
    
    int addr = EEPROM_DATA_START_ADDR + (count * EEPROM_RECORD_SIZE);
    EEPROM.put(addr, data);
    EEPROM.put(EEPROM_DATA_COUNT_ADDR, count + 1);
    EEPROM.commit();
    
    Serial.printf("Dữ liệu được lưu vào EEPROM. Tổng số bản ghi: %d\n", count + 1);
}

uint16_t getStoredDataCount() {
    uint16_t count;
    EEPROM.get(EEPROM_DATA_COUNT_ADDR, count);
    return count;
}


void clearEEPROMData() {
    Serial.println("Xóa dữ liệu EEPROM...");
    EEPROM.put(EEPROM_DATA_COUNT_ADDR, 0);
    EEPROM.commit();
    Serial.println("Đã reset bộ đếm EEPROM.");
}

void uploadStoredDataToFirebase() {
    // Kiểm tra Firebase status trực tiếp
    if (!app.ready() || WiFi.status() != WL_CONNECTED) {
        Serial.println("Firebase không sẵn sàng hoặc WiFi đã ngắt kết nối, không thể tải dữ liệu EEPROM");
        return;
    }
    
    uint16_t count = getStoredDataCount();
    if (count == 0) {
        Serial.println("Không có dữ liệu để tải từ EEPROM");
        return;
    }
    
    Serial.printf("Tải %d bản ghi từ EEPROM lên Firebase...\n", count);
    bool allUploaded = true;

    for (int i = 0; i < count; i++) {
        feedWatchdog();

        StoredData data;
        int addr = EEPROM_DATA_START_ADDR + (i * EEPROM_RECORD_SIZE);
        EEPROM.get(addr, data);
        
        // Tạo JSON payload (bỏ source, giữ timestamp từ ESP)
        FirebaseJson jsonPayload;
        jsonPayload.set("temperature", data.temperature);
        jsonPayload.set("humidity", data.humidity);
        jsonPayload.set("soil_moisture", data.soilMoisture);
        jsonPayload.set("light_level", data.lightLevel);
        jsonPayload.set("rain_detected", data.rainDetected);
        jsonPayload.set("pump_state", data.pumpState);
        jsonPayload.set("canopy_state", data.canopyState);
        jsonPayload.set("auto_mode", data.autoMode);
        
        // Thêm timestamp từ lúc record ở ESP
        time_t espTime = (data.timestamp > 1000000000) ? data.timestamp : time(nullptr);
        
        struct tm* timeinfo = localtime(&espTime);
        char timestampStr[25];
        strftime(timestampStr, sizeof(timestampStr), "%Y-%m-%d_%H:%M:%S", timeinfo);
        jsonPayload.set("timestamp", timestampStr);
        
        // Convert timestamp từ ESP thành format ngày (YYYY-MM-DD)
        char dateStr[11];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", timeinfo);

        // Tạo path với ngày từ timestamp của ESP
        String path = ROOT;
        path.concat("/sensors/history/");
        path.concat(dateStr);
        
        Serial.printf("Tải bản ghi %d/%d lên: %s\n", i+1, count, path.c_str());
        
        Database.push<object_t>(aClient, path, object_t(jsonPayload.raw()), processData, "upload EEPROM data");
        
        delay(500);
        
        // Kiểm tra Firebase connection sau mỗi upload
        if (!app.ready() || WiFi.status() != WL_CONNECTED) {
            Serial.println("Firebase đã ngắt kết nối trong khi tải, dừng...");
            allUploaded = false;
            return;
        }
    }
    
    // Xóa dữ liệu sau khi upload thành công
    if (allUploaded) {
        clearEEPROMData();
        Serial.println("Tất cả dữ liệu EEPROM đã được tải lên Firebase.");
    } else {
        Serial.println("Một phần dữ liệu chưa được tải (sẽ giữ lại EEPROM).");
    }
}

// AutoMode EEPROM functions
void saveAutoModeToEEPROM() {
    if (!systemState.eepromInitialized) return;
    
    // Save autoMode at address 3500
    EEPROM.put(EEPROM_AUTOMODE_ADDR, controlData.autoMode);
    EEPROM.commit();
    
    String status = controlData.autoMode ? "ON" : "OFF";
    String msg = "Chế độ tự động đã được lưu vào EEPROM: ";
    msg.concat(status);
    Serial.println(msg);
}

bool loadAutoModeFromEEPROM() {
    if (!systemState.eepromInitialized) {
        Serial.println("EEPROM chưa được khởi tạo, mặc định chế độ tự động là true");
        return true;
    }
    
    bool savedAutoMode;
    EEPROM.get(EEPROM_AUTOMODE_ADDR, savedAutoMode);
    
    String status = savedAutoMode ? "ON" : "OFF";
    String msg = "Chế độ tự động đã được tải từ EEPROM: ";
    msg.concat(status);
    Serial.println(msg);
    
    return savedAutoMode;
}