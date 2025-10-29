#include "config.h"
#include "sensor_handler.h"

void setupSensors() {
    Serial.print("Khởi tạo cảm biến... ");
    
    // Setup DHT11
    dht.begin();
    delay(1000);
    
    // Setup BH1750
    Wire.begin();
    lightMeter.begin();
    
    // Setup pins
    pinMode(SOIL_PIN, INPUT);
    pinMode(RAIN_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(CANOPY_PIN, OUTPUT);
    
    // Initialize outputs
    digitalWrite(PUMP_PIN, PUMP_OFF);
    digitalWrite(CANOPY_PIN, CANOPY_OFF);
    
    // Test sensors
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    float light = lightMeter.readLightLevel();
    
    if (isnan(temp) || isnan(hum)) {
        Serial.println("Lỗi DHT11!");
        sensorData.error = true;
        return;
    }
    
    if (light < 0) {
        Serial.println("Lỗi BH1750!");
        sensorData.error = true;
        return;
    }
    
    sensorData.initialized = true;
    Serial.println("Thành công!");
}

void readSensors() {
    unsigned long currentTime = millis();
    
    if (currentTime - sensorData.lastRead < SENSOR_READ_INTERVAL) {
        return;
    }
    sensorData.lastRead = currentTime;
    
    if (!sensorData.initialized) {
        return;
    }
    
    readDHT11();
    readSoilMoisture();
    readRainSensor();
    readLightSensor();
}

void readDHT11() {
    sensorData.temperature = dht.readTemperature();
    sensorData.humidity = dht.readHumidity();
    
    if (isnan(sensorData.temperature) || isnan(sensorData.humidity)) {
        Serial.println("Lỗi DHT11!");
        sensorData.error = true;
        return;
    }
    
    sensorData.error = false;
}

void readSoilMoisture() {
    // Quy từ thang 12-bit về 10-bit
    sensorData.soilMoisture = map(analogRead(SOIL_PIN), 4095, 0, 0, 1023); // map(x, in_min, in_max, out_min, out_max)

    if (sensorData.soilMoisture < 0 || sensorData.soilMoisture > 1023) {
        Serial.println("Lỗi cảm biến đất!");
        sensorData.error = true;
        return;
    }
    
    sensorData.error = false;
}

void readRainSensor() {
    int tmp = !digitalRead(RAIN_PIN);
    sensorData.rainDetected = tmp == 1 ? true : false;
}

void readLightSensor() {
    sensorData.lightLevel = lightMeter.readLightLevel();
    
    if (sensorData.lightLevel < 0) {
        Serial.println("Lỗi cảm biến ánh sáng!");
        sensorData.error = true;
        return;
    }
    
    sensorData.error = false;
}
