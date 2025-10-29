#include "config.h"
#include "model_final.h"
#include <HTTPClient.h>

// Cập nhật dữ liệu từ OpenWeatherMap Pro - hourly forecast cnt=1
void updateWeatherData() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < WEATHER_UPDATE_INTERVAL) return; // Update every 10 minutes
    lastUpdate = millis();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Weather: WiFi không kết nối");
        return;
    }

    String url = String(OPENWEATHER_HOST);
    url.concat("/data/2.5/forecast/hourly?lat=");
    url.concat(OPENWEATHER_LAT);
    url.concat("&lon=");
    url.concat(OPENWEATHER_LON);
    url.concat("&appid=");
    url.concat(API_KEY_WEATHER_API);
    url.concat("&units=");
    url.concat(OPENWEATHER_UNITS);
    url.concat("&cnt=");
    url.concat(OPENWEATHER_CNT);

    HTTPClient http;

    if (!http.begin(http_ssl_client, url)) {
        Serial.println("Weather: HTTP begin failed");
        return;
    }

    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("Weather: HTTP status %d\n", httpCode);
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    FirebaseJson json;
    json.setJsonData(payload);
    FirebaseJsonData data;

    float pop = 0.0f;
    float rain1h = 0.0f;

    if (json.get(data, "list/0/pop")) {
        pop = data.to<float>(); // 0..1
    }
    // list.rain["1h"] bị miss khi trời không mưa
    if (json.get(data, "list/0/rain/1h")) {
        rain1h = data.to<float>(); // mm
    }

    weatherData.popNext1h = pop * 100.0f; // store as percentage
    weatherData.rainNext1h = rain1h;
    weatherData.lastUpdate = millis();
    weatherData.initialized = true;

    Serial.printf("🌤️ Weather updated - Rain next 1h: %.2fmm, POP: %.0f%%\n",
                  weatherData.rainNext1h, weatherData.popNext1h);
}

// Hàm sử dụng mô hình XGBoost thật của bạn
void updateModelPrediction() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < WEATHER_UPDATE_INTERVAL) return; // Update every 10 minutes
    lastUpdate = millis();
    
    // Tạo instance của mô hình XGBoost
    Eloquent::ML::Port::XGBClassifier classifier;
    
    // Chuẩn bị input features cho mô hình
    // Dựa trên phân tích code, mô hình cần 3 features:
    float features[3];
    features[0] = static_cast<float>(sensorData.soilMoisture);
    features[1] = static_cast<float>(sensorData.temperature);
    features[2] = static_cast<float>(sensorData.humidity);
    
    // Chạy inference với mô hình XGBoost
    int prediction = classifier.predict(features);
    
    // Convert kết quả: 0 = không tưới, 1 = cần tưới
    modelPredict.needIrrigation = (prediction == 1);
    modelPredict.lastUpdate = millis();
    modelPredict.initialized = true;
    
    Serial.printf("🤖 XGBoost prediction: %s (features: soil=%.0f, temp=%.1f, hum=%.1f)\n", 
                  modelPredict.needIrrigation ? "NEED IRRIGATION" : "NO IRRIGATION",
                  features[0], features[1], features[2]);
}

// Hàm để test với mô hình XGBoost thật
void testWeatherScenarios() {
    Serial.println("\n=== Testing XGBoost Model Scenarios ===");
    
    // Tạo instance của mô hình
    Eloquent::ML::Port::XGBClassifier classifier;
    
    // Scenario 1: Đất khô, nhiệt độ cao, độ ẩm thấp
    float features1[3] = {800.0f, 35.0f, 25.0f}; // soil, temp, humidity
    int pred1 = classifier.predict(features1);
    Serial.printf("Scenario 1: Dry soil (%.0f), Hot (%.1f°C), Low humidity (%.1f%%) -> %s\n",
                  features1[0], features1[1], features1[2], 
                  pred1 == 1 ? "IRRIGATE" : "NO IRRIGATION");
    
    // Scenario 2: Đất ẩm, nhiệt độ thấp, độ ẩm cao
    float features2[3] = {300.0f, 20.0f, 80.0f};
    int pred2 = classifier.predict(features2);
    Serial.printf("Scenario 2: Wet soil (%.0f), Cool (%.1f°C), High humidity (%.1f%%) -> %s\n",
                  features2[0], features2[1], features2[2], 
                  pred2 == 1 ? "IRRIGATE" : "NO IRRIGATION");
    
    // Scenario 3: Đất trung bình, nhiệt độ trung bình, độ ẩm trung bình
    float features3[3] = {500.0f, 28.0f, 50.0f};
    int pred3 = classifier.predict(features3);
    Serial.printf("Scenario 3: Medium soil (%.0f), Warm (%.1f°C), Medium humidity (%.1f%%) -> %s\n",
                  features3[0], features3[1], features3[2], 
                  pred3 == 1 ? "IRRIGATE" : "NO IRRIGATION");
    
    Serial.println("=== End Testing ===\n");
}


