#include "MeteoConfigPortal.h"
#include <EEPROM.h>
#include <ESPmDNS.h>

#define EEPROM_SIZE 512
#define DEVICE_LIST_ADDR 50 // Сдвиг для конфигурационных данных
#define DEVICE_ID_ADDR 46  // Адрес хранения ID устройства (2 байта)

// ==== Конструктор ====
MeteoConfigPortal::MeteoConfigPortal() : server(80) {}

// ==== Функция запуска ====
void MeteoConfigPortal::begin() {
    // Подключение Wi-Fi через WiFiManager
    //WiFi.mode(WIFI_STA);
    wifiManager.resetSettings();
    wifiManager.setConfigPortalBlocking(false);    
    wifiManager.autoConnect("ConfigAP");

    EEPROM.begin(EEPROM_SIZE);
    deviceId = EEPROM.read(DEVICE_ID_ADDR) | (EEPROM.read(DEVICE_ID_ADDR + 1) << 8);    
    Serial.println("Device ID: " + String(deviceId));
    loadDevicesFromEEPROM();
    if (MDNS.begin("meteo"+String(deviceId))) {
        Serial.println("MDNS responder started: http://meteo"+String(deviceId)+".local");
    }
    //setupWebServer();
}

// ==== В цикле ====
void MeteoConfigPortal::loop() {
    doWiFiManager();
    /*if (configPortalRequested) {
        configPortalRequested = false;
        wifiManager.startConfigPortal("ConfigAP");
    }*/
}

// ==== Запуск WiFiManager ====
void MeteoConfigPortal::doWiFiManager() {
    if (configPortalRequested) {
        wifiManager.process(); // do processing

        /*// check for timeout
        if((millis()-startTime) > (timeout*1000)){
            Serial.println("portaltimeout");
            portalRunning = false;
            wifiManager.stopConfigPortal();            
        }*/
    }
    if(!webPortalActive){
        if(WiFi.status() == WL_CONNECTED){         
            webPortalActive = true;
            configPortalRequested = false;
            //wifiManager.stopConfigPortal();            
            Serial.println("Web portal started");
            setupWebServer();
        }
    } 
}

// ==== Сохранение устройств в EEPROM ====
void MeteoConfigPortal::saveDevicesToEEPROM() {
    int addr = DEVICE_LIST_ADDR;
    int count = meteoDevices.size();
    Serial.println("Devices count: " + String(count));
    EEPROM.write(addr++, count);
    for (const auto &dev : meteoDevices) {
        EEPROM.write(addr++, dev.id);
        for (char c : dev.name) {
            EEPROM.write(addr++, c);
        }
        EEPROM.write(addr++, '\0');
    }
    if(!EEPROM.commit()) {
        Serial.println("Ошибка записи в EEPROM");
    }
    else {
        Serial.println("Устройства сохранены в EEPROM");
    }
}

// ==== Загрузка устройств из EEPROM ====
void MeteoConfigPortal::loadDevicesFromEEPROM() {
    meteoDevices.clear();
    int addr = DEVICE_LIST_ADDR;
    int count = EEPROM.read(addr++);
    Serial.println("Devices count: " + String(count));
    if (count < 0 || count > (EEPROM_SIZE - DEVICE_LIST_ADDR) / sizeof(MeteoDevice)) return; // Проверка на корректность данных
    
    for (int i = 0; i < count; i++) {
        int id = EEPROM.read(addr++);
        String name;
        char c;
        while ((c = EEPROM.read(addr++)) != '\0') {
            name += c;
        }
        meteoDevices.push_back({id, name});
    }
}

// ==== Получение списка устройств в виде строки ====
String MeteoConfigPortal::getMeteoDevicesList() {
    String output = "Devices:\n";
    for (const auto &dev : meteoDevices) {
        output += "ID: " + String(dev.id) + ", Name: " + dev.name + "\n";
    }
    return output;
}

// ==== Сохранение ID устройства ====
void MeteoConfigPortal::saveDeviceId(int id) {
    EEPROM.write(DEVICE_ID_ADDR, id & 0xFF);
    EEPROM.write(DEVICE_ID_ADDR + 1, (id >> 8) & 0xFF);
    EEPROM.commit();
    deviceId = id;
}

// ==== Обработчик установки ID ====
void MeteoConfigPortal::handleSetDeviceId(AsyncWebServerRequest *request) {
    if (!request->hasParam("deviceId")) {
        request->send(400, "text/plain", "Ошибка: нужен параметр deviceId");
        return;
    }

    int id = request->getParam("deviceId")->value().toInt();
    if(id < 0 || id > 0xFFFF) {
        request->send(400, "text/plain", "Ошибка: некорректный ID устройства");
        return;
    }
    saveDeviceId(id);
    request->send(200, "text/plain", "ID устройства сохранен");
}

// ==== Функция получения текущего ID ====
String MeteoConfigPortal::getDeviceId() {
    return String(deviceId);
}

// ==== Обработчик запроса на получение списка устройств ====
void MeteoConfigPortal::handleGetDevices(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", getMeteoDevicesList());
}

// ==== Обработчик добавления устройства ====
void MeteoConfigPortal::handleAddDevice(AsyncWebServerRequest *request) {
    if (!request->hasParam("id") || !request->hasParam("name")) {
        request->send(400, "text/plain", "Ошибка: нужны параметры id и name");
        return;
    }

    int id = request->getParam("id")->value().toInt();
    String name = request->getParam("name")->value();

    meteoDevices.push_back({id, name});
    saveDevicesToEEPROM();
    request->send(200, "text/plain", "Устройство добавлено");
}

// ==== Обработчик удаления устройства ====
void MeteoConfigPortal::handleRemoveDevice(AsyncWebServerRequest *request) {
    if (!request->hasParam("id")) {
        request->send(400, "text/plain", "Ошибка: нужен параметр id");
        return;
    }

    int id = request->getParam("id")->value().toInt();
    meteoDevices.erase(std::remove_if(meteoDevices.begin(), meteoDevices.end(),
        [id](MeteoDevice &dev) { return dev.id == id; }), meteoDevices.end());
    saveDevicesToEEPROM();
    request->send(200, "text/plain", "Устройство удалено");
}

// ==== Запуск конфигурационного портала ====
void MeteoConfigPortal::handleStartConfigPortal(AsyncWebServerRequest *request) {
    configPortalRequested = true;
    request->send(200, "text/plain", "Starting config portal...");
}

// ==== Встроенный HTML-фронтенд ====
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Meteo Config</title>
    <script>
        async function loadDevices() {
            let response = await fetch("/devices");
            let data = await response.text();
            document.getElementById("deviceList").innerText = data;

            let deviceIdResponse = await fetch("/getDeviceId");
            let deviceId = await deviceIdResponse.text();
            document.getElementById("deviceId").value = deviceId;
        }

        async function addDevice() {
            let id = document.getElementById("id").value;
            let name = document.getElementById("name").value;
            await fetch(`/add?id=${id}&name=${name}`);
            loadDevices();
        }

        async function removeDevice() {
            let id = document.getElementById("id").value;
            await fetch(`/remove?id=${id}`);
            loadDevices();
        }

        async function setDeviceId() {
            let deviceId = document.getElementById("deviceId").value;
            await fetch(`/setDeviceId?deviceId=${deviceId}`);
        }


        async function startWiFiConfig() {
            await fetch("/startConfig");
        }
        window.onload = loadDevices;
    </script>
</head>
<body>
    <h1>Meteo Config</h1>
    <pre id="deviceList"></pre>
    <input id="id" placeholder="ID">
    <input id="name" placeholder="Name">
    <button onclick="addDevice()">Add</button>
    <button onclick="removeDevice()">Delete</button>
    <br><br>
    <label for="deviceId">Device ID:</label>
    <input id="deviceId" placeholder="Device ID">
    <button onclick="setDeviceId()">Change Device ID</button>
    <br><br>
    <button onclick="startWiFiConfig()">Open WiFi Config</button>
</body>
</html>
)rawliteral";

// ==== Настройка веб-сервера ====
void MeteoConfigPortal::setupWebServer() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/devices", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleGetDevices(request);
    });

    server.on("/add", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAddDevice(request);
    });

    server.on("/remove", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRemoveDevice(request);
    });

    server.on("/setDeviceId", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleSetDeviceId(request);
    });

    server.on("/getDeviceId", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", getDeviceId());
    });

    server.on("/startConfig", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStartConfigPortal(request);
    });

    server.begin();
}