#include "config.h"
#include "firebase_handler.h"
#include "system_handler.h"
#include "auto_control.h"

void setupFirebase(){
    Serial.println("Cấu hình Firebase...");
    
    // Kiểm tra WiFi và SSL client trước khi cấu hình Firebase
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi không kết nối! Không thể cấu hình Firebase.");
        Serial.println("Hãy kết nối WiFi trước với lệnh 'w'");
        return;
    }
    
    // Cấu hình SSL client
    Serial.println("Đang cấu hình SSL client...");
    fb_ssl_client.setInsecure();
    fb_ssl_client.setTimeout(30000);
    fb_ssl_client.setHandshakeTimeout(30);
    Serial.println("SSL client đã được cấu hình");
    
    initializeApp(aClient, app, getAuth(user_auth), processData, "FirebaseAuth");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    
    Serial.println("Cấu hình Firebase hoàn tất!");
    Serial.println("=======================");
}

void uploadSensorData() {
    if (sensorData.error) {
        Serial.println("Lỗi cảm biến! Không thể tải dữ liệu cảm biến.");
        return;
    }

    if (!firebaseConnected) {
        Serial.println("Firebase không kết nối. Lưu vào EEPROM.");
        saveDataToEEPROM();
        return;
    }

    FirebaseJson jsonPayload;
    String timestamp = getISOTimestamp();

    jsonPayload.set("temperature", sensorData.temperature);
    jsonPayload.set("humidity", sensorData.humidity);
    jsonPayload.set("soil_moisture", sensorData.soilMoisture);
    jsonPayload.set("light_level", sensorData.lightLevel);
    jsonPayload.set("rain_detected", sensorData.rainDetected);
    jsonPayload.set("pump_state", controlData.pumpState);
    jsonPayload.set("canopy_state", controlData.canopyState);
    jsonPayload.set("auto_mode", controlData.autoMode);
    jsonPayload.set("timestamp", timestamp);

    object_t payloadObject(jsonPayload.raw());

    // Dữ liệu hiện tại
    String currentPath = ROOT;
    currentPath += "/sensors/current";
    Serial.printf("Setting current sensor data at: %s\n", currentPath.c_str());
    Database.set<object_t>(aClient, currentPath, payloadObject, processData, "Update current sensors");

    // Dữ liệu lịch sử
    char datePath[11];
    strftime(datePath, sizeof(datePath), "%Y-%m-%d", &systemState.timeinfo);

    String historyPath = ROOT;
    historyPath.concat("/sensors/history/");
    historyPath.concat(datePath);

    Serial.printf("Pushing new sensor record to: %s\n", historyPath.c_str());
    Database.push<object_t>(aClient, historyPath, payloadObject, processData, "Upload sensor history");
}

void uploadSystemStatus() {
    // Kiểm tra Firebase connection thực tế
    if (!firebaseConnected || !app.ready() || WiFi.status() != WL_CONNECTED) {
        Serial.println("Firebase không kết nối. Không thể tải trạng thái hệ thống.");
        return;
    }
    
    FirebaseJson statusJson;
    statusJson.set("last_update", getISOTimestamp());
    statusJson.set("uptime", systemState.uptimeSeconds);
    statusJson.set("wifi_strength", WiFi.RSSI());
    statusJson.set("auto_mode", controlData.autoMode);
    statusJson.set("pump_state", controlData.pumpState);
    statusJson.set("canopy_state", controlData.canopyState);
    
    // Trạng thái cảm biến
    FirebaseJson sensorStatus;
    sensorStatus.set("dht11", !isnan(sensorData.temperature) ? "ok" : "error");
    sensorStatus.set("soil_sensor", "ok");
    sensorStatus.set("rain_sensor", "ok");
    sensorStatus.set("light_sensor", sensorData.lightLevel > 0 ? "ok" : "error");
    
    statusJson.set("sensors", sensorStatus);
    
    // Cập nhật trạng thái
    String path = ROOT;
    path += "/system/status";
    Database.set<object_t>(aClient, path, object_t(statusJson.raw()), processData, "System status update");
}

void uploadAlerts(String alertType, String message) {
    // Kiểm tra Firebase connection thực tế
    if (!firebaseConnected || !app.ready() || WiFi.status() != WL_CONNECTED) {
        Serial.println("Firebase không kết nối! Không thể tải cảnh báo.");
        return;
    }
    
    FirebaseJson alertJson;
    alertJson.set("type", alertType);
    alertJson.set("message", message);
    alertJson.set("timestamp", getISOTimestamp());
    alertJson.set("severity", "warning");
    alertJson.set("count_today", alertData.alertCountToday);
    
    String path = ROOT;
    path += "/alerts/current/";
    path += alertType;
    Database.set<object_t>(aClient, path, object_t(alertJson.raw()), processData, "Upload alert");
    
    char datePath[11];
    strftime(datePath, sizeof(datePath), "%Y-%m-%d", &systemState.timeinfo);
    String historyPath = ROOT;
    historyPath.concat("/alerts/history/");
    historyPath.concat(datePath);
    historyPath.concat("/");
    historyPath.concat(alertType);
    
    Database.push<object_t>(aClient, historyPath, object_t(alertJson.raw()), processData, "Upload alert history");
}

void uploadControlStatus() {
    // Kiểm tra Firebase connection thực tế
    if (!firebaseConnected || !app.ready() || WiFi.status() != WL_CONNECTED) {
        Serial.println("Firebase không kết nối! Không thể tải trạng thái điều khiển.");
        return;
    }
    
    FirebaseJson controlJson;
    controlJson.set("pump_state", controlData.pumpState ? "ON" : "OFF");
    controlJson.set("canopy_state", controlData.canopyState ? "ON" : "OFF");
    controlJson.set("auto_mode", controlData.autoMode);
    controlJson.set("timestamp", getISOTimestamp());
    
    String path = ROOT;
    path += "/controls/current";
    Database.set<object_t>(aClient, path, object_t(controlJson.raw()), processData, "update controls");
}

void processData(AsyncResult &aResult){
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

    if (aResult.isDebug())
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

    if (aResult.isError())
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

    if (aResult.available())
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}

void processControlCommands(AsyncResult &aResult) {
    if (!aResult.isResult()) {
        return;
    }

    if (aResult.available()) {
        RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();
        
        if (stream.isStream()) {
            String path = stream.dataPath();
            String dataStr = stream.to<String>();

            // Xử lý khi path là root "/" - nhận toàn bộ JSON object
            if (path == "/" || path.isEmpty()) {
                FirebaseJson json;
                if (json.setJsonData(dataStr)) {
                    FirebaseJsonData data;
                    
                    if (json.get(data, "pump_state")) {
                        String state = data.to<String>();
                        setPumpState(state == "ON");
                        Serial.printf("Bơm: %s\n", state.c_str());
                    }
                    if (json.get(data, "canopy_state")) {
                        String state = data.to<String>();
                        setCanopyState(state == "ON");
                        Serial.printf("Mái che: %s\n", state.c_str());
                    }
                    if (json.get(data, "auto_mode")) {
                        bool autoMode = data.to<bool>();
                        controlData.autoMode = autoMode;
                        saveAutoModeToEEPROM();
                        Serial.printf("Auto mode: %s\n", autoMode ? "ON" : "OFF");
                    }
                } else {
                    Serial.println("Lỗi parse JSON!");
                }
            }
            // Xử lý khi path là con của controls/current (ví dụ: /pump_state)
            else {
                if (path.indexOf("pump_state") >= 0) {
                    setPumpState(dataStr == "ON");
                    Serial.printf("Bơm: %s\n", dataStr.c_str());
                }
                else if (path.indexOf("canopy_state") >= 0) {
                    setCanopyState(dataStr == "ON");
                    Serial.printf("Mái che: %s\n", dataStr.c_str());
                }
                else if (path.indexOf("auto_mode") >= 0) {
                    bool autoMode = false;
                    if (dataStr == "true") {
                        autoMode = true;
                    } else {
                        autoMode = false;
                    }
                    controlData.autoMode = autoMode;
                    saveAutoModeToEEPROM();
                    Serial.printf("Auto mode: %s\n", autoMode ? "ON" : "OFF");
                }
            }
        }
    }
}

void startControlStream() {
    if (WiFi.status() != WL_CONNECTED || !app.ready()) {
        Serial.println("Không thể khởi động luồng điều khiển: WiFi hoặc Firebase chưa sẵn sàng");
        return;
    }
    
    Serial.println("Khởi động luồng điều khiển Firebase Realtime...");
    String path = ROOT;
    path += "/controls/current";
    Database.get(aClient, path, processControlCommands, true /*SSE*/, "controlStreamTask");
}
