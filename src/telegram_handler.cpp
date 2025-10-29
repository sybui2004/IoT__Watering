#include "telegram_handler.h"
#include "system_handler.h"   
// Global Telegram Bot object - will be initialized after WiFi connection
UniversalTelegramBot* telegramBot = nullptr;

bool isAuthorizedChat(const String& chatId) {
    for (int i = 0; i < TELEGRAM_CHAT_COUNT; i++) {
        if (chatId == TELEGRAM_CHAT_IDS[i]) return true;
    }
    return false;
}

void setupTelegramBot() {
    Serial.println("Đang kết nối Telegram Bot...");
    
    // Check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi không kết nối, không thể kết nối Telegram Bot");
        return;
    }
    
    // Initialize bot with the dedicated Telegram SSL client
    telegramBot = new UniversalTelegramBot(TELEGRAM_BOT_TOKEN, tg_ssl_client);
    feedWatchdog();
    
    // Set the bot token explicitly
    telegramBot->setMyCommands("start - Bắt đầu bot\nhelp - Hiển thị trợ giúp\nstatus - Trạng thái hệ thống\nsensors - Dữ liệu cảm biến");
    feedWatchdog();
    
    // Send startup message
    String startupMsg = "🤖 Smart Irrigation System đã khởi động!\n";
    startupMsg.concat("Sử dụng /help để xem các lệnh có sẵn");
    sendTelegramMessage(startupMsg);
    feedWatchdog();
    
    Serial.println("Telegram Bot kết nối thành công!");
    
    delay(500); // Giảm từ 1000ms xuống 500ms
    feedWatchdog();
    
    Serial.println("Kiểm tra kết nối bot...");
    bool botConnected = telegramBot->getMe();
    feedWatchdog();
    Serial.println(botConnected ? "Kết nối bot: THÀNH CÔNG" : "Kết nối bot: THẤT BẠI");

    // Lần đầu khởi động, xóa toàn bộ tin cũ
    Serial.println("Xóa tin nhắn cũ...");
    int oldMsgCount = telegramBot->getUpdates(0);
    feedWatchdog();
    if (oldMsgCount > 0) {
        telegramBot->last_message_received = telegramBot->messages[oldMsgCount - 1].update_id + 1;
        Serial.printf("Bỏ qua %d tin nhắn cũ. Bắt đầu mới từ update_id = %d\n",
                      oldMsgCount, telegramBot->last_message_received);
    } else {
        Serial.println("Không có tin nhắn cũ để bỏ qua.");
    }
    feedWatchdog();
}


void sendTelegramMessageToChat(const String& chatId, const String& message) {
    if (WiFi.status() == WL_CONNECTED && telegramBot != nullptr) {
        telegramBot->sendMessage(chatId, message, "");
        Serial.printf("Tin nhắn Telegram đã được gửi đến %s: %s\n", chatId.c_str(), message.c_str());
    }
}

void sendTelegramMessage(const String& message) {
    if (WiFi.status() == WL_CONNECTED && telegramBot != nullptr) {
        for (int i = 0; i < TELEGRAM_CHAT_COUNT; i++) {
            telegramBot->sendMessage(TELEGRAM_CHAT_IDS[i], message, "");
            Serial.printf("Tin nhắn đã gửi đến chat_id[%d]: %s\n", i, TELEGRAM_CHAT_IDS[i].c_str());
        }
    }
}

void handleTelegramMessages() {
    static unsigned long lastCheck = 0;
    static bool wifiNotConnectedPrinted = false;
    static bool botNotInitializedPrinted = false;
    
    unsigned long checkInterval = (WiFi.status() == WL_CONNECTED && telegramBot != nullptr) 
                                 ? TELEGRAM_UPDATE_INTERVAL 
                                 : TELEGRAM_UPDATE_INTERVAL * 10; 

    if (millis() - lastCheck >= checkInterval) {
        if (WiFi.status() != WL_CONNECTED) {
            if (!wifiNotConnectedPrinted) {
                Serial.println("WiFi không kết nối, bỏ qua kiểm tra Telegram");
                wifiNotConnectedPrinted = true;
            }
            lastCheck = millis();
            return;
        } else {
            wifiNotConnectedPrinted = false; 
        }

        if (telegramBot == nullptr) {
            if (!botNotInitializedPrinted) {
                Serial.println("Telegram Bot chưa được khởi tạo, bỏ qua kiểm tra");
                botNotInitializedPrinted = true;
            }
            lastCheck = millis();
            return;
        } else {
            botNotInitializedPrinted = false; 
        }

        // Lấy tin nhắn mới
        int numNewMessages = telegramBot->getUpdates(telegramBot->last_message_received + 1);

        if (numNewMessages > 0) {
            Serial.printf("Nhận được %d tin nhắn Telegram mới\n", numNewMessages);

            for (int i = 0; i < numNewMessages; i++) {
                String message = telegramBot->messages[i].text;
                String chatId = telegramBot->messages[i].chat_id;
                String username = telegramBot->messages[i].from_name;

                Serial.printf("Tin nhắn từ %s: %s\n", username.c_str(), message.c_str());

                if (message.startsWith("/")) {
                    handleTelegramCommand(message, chatId);
                } else {
                    String response = "Xin chào ";
                    response.concat(username);
                    response.concat("! Gõ /help để xem lệnh.");
                    sendTelegramMessageToChat(chatId, response);
                }
            }

            // Cập nhật offset
            telegramBot->last_message_received = telegramBot->messages[numNewMessages - 1].update_id;
            telegramBot->getUpdates(telegramBot->last_message_received);
        } else {
            static unsigned long lastConnectionTest = 0;
            if (millis() - lastConnectionTest >= 30000) { // test mỗi 30s
                telegramBot->getMe();
                lastConnectionTest = millis();
            }
        }

        lastCheck = millis();
    }
}




void handleTelegramCommand(const String& command, const String& chatId) {
    if (!isAuthorizedChat(chatId)) {
        sendTelegramMessageToChat(chatId, "🚫 Bạn không có quyền điều khiển hệ thống này!");
        Serial.printf("Từ chối truy cập từ ChatID: %s\n", chatId.c_str());
        return;
    }

    Serial.printf("Lệnh hợp lệ từ ChatID %s: %s\n", chatId.c_str(), command.c_str());
    
    if (command == "/help") {
        Serial.println("Processing /help command");
        handleHelpCommand(chatId);
    }
    else if (command == "/start") {
        Serial.println("Processing /start command");
        String welcomeMsg = "🤖 Chào mừng bạn đến với Smart Irrigation System!\n";
        welcomeMsg.concat("Sử dụng /help để xem các lệnh có sẵn");
        sendTelegramMessageToChat(chatId, welcomeMsg);
    }
    else if (command == "/status") {
        Serial.println("Processing /status command");
        handleStatusCommand(chatId);
    }
    else if (command == "/sensors") {
        Serial.println("Processing /sensors command");
        String sensorMsg = getSensorDataText();
        sendTelegramMessageToChat(chatId, sensorMsg);
    }
    else if (command.startsWith("/pump")) {
        Serial.println("Processing /pump command");
        String action = command.substring(6); // Remove "/pump "
        handlePumpCommand(action, chatId);
    }
    else if (command.startsWith("/canopy")) {
        Serial.println("Processing /canopy command");
        String action = command.substring(8); // Remove "/canopy "
        handleCanopyCommand(action, chatId);
    }
    else if (command.startsWith("/auto")) {
        Serial.println("Processing /auto command");
        String action = command.substring(6); // Remove "/auto "
        handleAutoModeCommand(action, chatId);
    }
    else {
        String unknownCmd = "Unknown command: ";
        unknownCmd.concat(command);
        Serial.println(unknownCmd);
        sendTelegramMessageToChat(chatId, "❌ Lệnh không hợp lệ! Sử dụng /help để xem các lệnh có sẵn.");
    }
}

void handleHelpCommand(const String& chatId) {
    Serial.println("handleHelpCommand called");
    String helpMsg = "🤖 Smart Irrigation System - Danh sách lệnh:\n\n";
    helpMsg.concat("📊 Thông tin hệ thống:\n");
    helpMsg.concat("• /status - Trạng thái hệ thống\n");
    helpMsg.concat("• /sensors - Dữ liệu cảm biến\n");
    helpMsg.concat("• /help - Hiển thị trợ giúp\n\n");
    helpMsg.concat("💧 Điều khiển bơm:\n");
    helpMsg.concat("• /pump on - Bật bơm\n");
    helpMsg.concat("• /pump off - Tắt bơm\n");
    helpMsg.concat("• /pump status - Trạng thái bơm\n\n");
    helpMsg.concat("🏠 Điều khiển mái che:\n");
    helpMsg.concat("• /canopy on - Bật mái che\n");
    helpMsg.concat("• /canopy off - Tắt mái che\n");
    helpMsg.concat("• /canopy status - Trạng thái mái che\n\n");
    helpMsg.concat("🤖 Chế độ tự động:\n");
    helpMsg.concat("• /auto on - Bật chế độ tự động\n");
    helpMsg.concat("• /auto off - Tắt chế độ tự động\n");
    helpMsg.concat("• /auto status - Trạng thái chế độ tự động");
    
    Serial.println("Gửi tin nhắn trợ giúp đến chatId: ");
    Serial.println(chatId);
    sendTelegramMessageToChat(chatId, helpMsg);
}

void handleStatusCommand(const String& chatId) {
    String statusMsg = getSystemStatusText();
    sendTelegramMessageToChat(chatId, statusMsg);
}

void sendSystemStatus() {
    String statusMsg = getSystemStatusText();
    sendTelegramMessage(statusMsg);
}

void sendSensorData() {
    String sensorMsg = getSensorDataText();
    sendTelegramMessage(sensorMsg);
}

void handlePumpCommand(const String& action, const String& chatId) {
    if (action == "on") {
        digitalWrite(PUMP_PIN, PUMP_ON);
        controlData.pumpState = true;
        controlData.lastPumpOn = millis();
        sendTelegramMessageToChat(chatId, "💧 Bơm đã được BẬT");
        Serial.println("Bơm đã được BẬT qua Telegram");
    }
    else if (action == "off") {
        digitalWrite(PUMP_PIN, PUMP_OFF);
        controlData.pumpState = false;
        sendTelegramMessageToChat(chatId, "💧 Bơm đã được TẮT");
        Serial.println("Bơm đã được TẮT qua Telegram");
    }
    else if (action == "status") {
        String pumpStatus = controlData.pumpState ? "BẬT" : "TẮT";
        String response = "💧 Trạng thái bơm: ";
        response.concat(pumpStatus);
        sendTelegramMessageToChat(chatId, response);
    }
    else {
        sendTelegramMessageToChat(chatId, "❌ Lệnh bơm không hợp lệ! Sử dụng: /pump on, /pump off, hoặc /pump status");
    }
}

void handleCanopyCommand(const String& action, const String& chatId) {
    if (action == "on") {
        digitalWrite(CANOPY_PIN, CANOPY_ON);
        controlData.canopyState = true;
        controlData.lastCanopyOn = millis();
        sendTelegramMessageToChat(chatId, "🏠 Mái che đã được BẬT");
        Serial.println("Mái che đã được BẬT qua Telegram");
    }
    else if (action == "off") {
        digitalWrite(CANOPY_PIN, CANOPY_OFF);
        controlData.canopyState = false;
        sendTelegramMessageToChat(chatId, "🏠 Mái che đã được TẮT");
        Serial.println("Mái che đã được TẮT qua Telegram");
    }
    else if (action == "status") {
        String canopyStatus = controlData.canopyState ? "BẬT" : "TẮT";
        String response = "🏠 Trạng thái mái che: ";
        response.concat(canopyStatus);
        sendTelegramMessageToChat(chatId, response);
    }
    else {
        sendTelegramMessageToChat(chatId, "❌ Lệnh mái che không hợp lệ! Sử dụng: /canopy on, /canopy off, hoặc /canopy status");
    }
}

void handleAutoModeCommand(const String& action, const String& chatId) {
    if (action == "on") {
        controlData.autoMode = true;
        sendTelegramMessageToChat(chatId, "🤖 Chế độ tự động đã được BẬT");
        saveAutoModeToEEPROM(); // Save to EEPROM
        Serial.println("Chế độ tự động đã được BẬT qua Telegram");
    }
    else if (action == "off") {
        controlData.autoMode = false;
        sendTelegramMessageToChat(chatId, "🤖 Chế độ tự động đã được TẮT");
        saveAutoModeToEEPROM(); // Save to EEPROM
        Serial.println("Chế độ tự động đã được TẮT qua Telegram");
    }
    else if (action == "status") {
        String autoStatus = controlData.autoMode ? "BẬT" : "TẮT";
        String response = "🤖 Trạng thái chế độ tự động: ";
        response.concat(autoStatus);
        sendTelegramMessageToChat(chatId, response);
    }
    else {
        sendTelegramMessageToChat(chatId, "❌ Lệnh chế độ tự động không hợp lệ! Sử dụng: /auto on, /auto off, hoặc /auto status");
    }
}

String getSystemStatusText() {
    String status = "📊 Trạng thái hệ thống:\n\n";
    
    // System info
    status.concat("🕐 Thời gian: ");
    status.concat(String(systemState.timeinfo.tm_hour));
    status.concat(":");
    status.concat(String(systemState.timeinfo.tm_min).substring(0,2));
    status.concat("\n");
    
    status.concat("⏱️ Uptime: ");
    status.concat(String(systemState.uptimeSeconds / 3600));
    status.concat("h ");
    status.concat(String((systemState.uptimeSeconds % 3600) / 60));
    status.concat("m\n");
    
    status.concat("📶 WiFi: ");
    status.concat(String(WiFi.status() == WL_CONNECTED ? "Kết nối" : "Mất kết nối"));
    status.concat("\n");
    
    status.concat("🔥 Firebase: ");
    status.concat(String(firebaseConnected ? "Kết nối" : "Mất kết nối"));
    status.concat("\n\n");
    
    // Control status
    status.concat(getControlStatusText());
    
    return status;
}

String getSensorDataText() {
    String sensorMsg = "📡 Dữ liệu cảm biến:\n\n";
    
    sensorMsg.concat("🌡️ Nhiệt độ: ");
    sensorMsg.concat(String(sensorData.temperature, 1));
    sensorMsg.concat("°C\n");
    
    sensorMsg.concat("💧 Độ ẩm: ");
    sensorMsg.concat(String(sensorData.humidity, 1));
    sensorMsg.concat("%\n");
    
    sensorMsg.concat("🌱 Độ ẩm đất: ");
    sensorMsg.concat(String(sensorData.soilMoisture));
    sensorMsg.concat("\n");
    
    sensorMsg.concat("☀️ Ánh sáng: ");
    sensorMsg.concat(String(sensorData.lightLevel, 0));
    sensorMsg.concat(" lux\n\n");
    
    sensorMsg.concat("☔ Mưa: ");
    sensorMsg.concat(String(sensorData.rainDetected ? "Có" : "Không"));
    sensorMsg.concat("\n");
    
    // Weather data
    if (weatherData.initialized) {
        sensorMsg.concat("🌤️ Dự báo thời tiết:\n");
        sensorMsg.concat("🌧️ Mưa 1h tới: ");
        sensorMsg.concat(String(weatherData.rainNext1h, 1));
        sensorMsg.concat("mm\n");
        sensorMsg.concat("📊 Xác suất mưa: ");
        sensorMsg.concat(String(weatherData.popNext1h * 100, 0));
        sensorMsg.concat("%\n\n");
    }
    
    // Model prediction
    if (modelPredict.initialized) {
        sensorMsg.concat("🤖 Dự đoán AI:\n");
        sensorMsg.concat("💧 Cần tưới: ");
        sensorMsg.concat(String(modelPredict.needIrrigation ? "Có" : "Không"));
        sensorMsg.concat("\n");
    }
    
    return sensorMsg;
}

String getControlStatusText() {
    String control = "🎛️ Trạng thái điều khiển:\n\n";
    
    control.concat("💧 Bơm: ");
    control.concat(String(controlData.pumpState ? "BẬT" : "TẮT"));
    control.concat("\n");
    
    control.concat("🏠 Mái che: ");
    control.concat(String(controlData.canopyState ? "BẬT" : "TẮT"));
    control.concat("\n");
    
    control.concat("🤖 Tự động: ");
    control.concat(String(controlData.autoMode ? "BẬT" : "TẮT"));
    control.concat("\n");
    
    // Show timing info
    if (controlData.pumpState) {
        unsigned long pumpTime = (millis() - controlData.lastPumpOn) / 1000;
        control.concat("⏱️ Bơm đã chạy: ");
        control.concat(String(pumpTime));
        control.concat("s\n");
    }
    
    if (controlData.canopyState) {
        unsigned long canopyTime = (millis() - controlData.lastCanopyOn) / 1000;
        control.concat("⏱️ Mái che đã chạy: ");
        control.concat(String(canopyTime));
        control.concat("s\n");
    }
    
    return control;
}
