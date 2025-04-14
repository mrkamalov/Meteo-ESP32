#include "MeteoConfigPortal.h"
#include <EEPROM.h>
#include <ESPmDNS.h>

// ==== Конструктор ====
MeteoConfigPortal::MeteoConfigPortal() : server(80) {}

#define CLEAR_EEPROM false  // Поставь true для очистки EEPROM при старте

// ==== Функция запуска ====
void MeteoConfigPortal::begin() {
    EEPROM.begin(EEPROM_SIZE);
    if (CLEAR_EEPROM) {
        for (int i = 10; i < EEPROM_SIZE; i++) {
            EEPROM.write(i, 0);
        }
        EEPROM.commit();
    }
    deviceId = EEPROM.read(DEVICE_ID_ADDR) | (EEPROM.read(DEVICE_ID_ADDR + 1) << 8);    
    Serial.println("Device ID: " + String(deviceId));
    loadWiFiSettings();
    loadDevicesFromEEPROM();
    if (connectToWiFi()) {
        Serial.println("Connected to WiFi: " + wifiSSID);
        Serial.println("IP Address: " + WiFi.localIP().toString());
    }
    else{
        Serial.println("Failed to connect to WiFi. Starting AP...");
        startAccessPoint();
    }
    if (MDNS.begin("meteo"+String(deviceId))) {
        Serial.println("MDNS responder started: http://meteo"+String(deviceId)+".local");
    }
    setupWebServer();
    ElegantOTA.begin(&server);
    server.begin();
}

void MeteoConfigPortal::loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (apActive) {
            WiFi.softAPdisconnect(true);
            apActive = false;
            Serial.println("Disconnected AP due to WiFi STA connection");
            Serial.println("Connected to WiFi: " + wifiSSID);
            Serial.println("IP Address: " + WiFi.localIP().toString());            
        }
    }
    else{
        unsigned long currentMillis = millis();        

        // Цикл точки доступа — выключить/включить, если никого нет
        if (apActive) {
            if(currentMillis - lastApCheck > apCheckInterval){
                lastApCheck = currentMillis;
                Serial.println("Checking AP status...");
                if (WiFi.softAPgetStationNum() == 0) {
                    Serial.println("Cycling AP: No clients connected");
                    WiFi.softAPdisconnect(true);
                    apActive = false;
                    apRestartPending = true;
                    apRestartTime = currentMillis;
                    lastWifiCheck = currentMillis; // Сброс таймера WiFi проверки
                }
                else{
                    Serial.println("AP active: " + String(WiFi.softAPgetStationNum()) + " clients connected");
                }
            }            
        }
        else{
            // Проверка подключения к WiFi
            if (currentMillis - lastWifiCheck > wifiCheckInterval) {
                lastWifiCheck = currentMillis;
                Serial.println("Restarting WiFi AP...");
                startAccessPoint();
            }
            // Отложенный запуск точки доступа
            if (apRestartPending && currentMillis - apRestartTime >= apRestartDelay) {
                startAccessPoint();
                apRestartPending = false;
            }
        }        
    }    
}

void MeteoConfigPortal::startAccessPoint() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("MeteoConfig"+String(deviceId));
    apActive = true;
    lastApCheck = millis();
    Serial.println("Started Access Point");
}


bool MeteoConfigPortal::connectToWiFi() {
    if (wifiSSID.isEmpty()) return false;
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    return WiFi.status() == WL_CONNECTED;
}

void MeteoConfigPortal::loadWiFiSettings() {
    char ssid[20], pass[20];
    for (int i = 0; i < 20; ++i) {
        ssid[i] = EEPROM.read(WIFI_SSID_ADDR + i);
        pass[i] = EEPROM.read(WIFI_PASS_ADDR + i);
    }
    wifiSSID = String(ssid);
    wifiPass = String(pass);
}

void MeteoConfigPortal::saveWiFiSettings(const String& ssid, const String& pass) {
    for (int i = 0; i < 20; ++i) {
        EEPROM.write(WIFI_SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
        EEPROM.write(WIFI_PASS_ADDR + i, i < pass.length() ? pass[i] : 0);
    }
    EEPROM.commit();
    wifiSSID = ssid;
    wifiPass = pass;
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

// ==== Обработчик настроек WiFi ====
void MeteoConfigPortal::handleWiFiSettings(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid") && request->hasParam("pass")) {
        String ssid = request->getParam("ssid")->value();
        String pass = request->getParam("pass")->value();
        saveWiFiSettings(ssid, pass);
        request->send(200, "text/plain", "WiFi settings saved. Rebooting...");
        delay(1000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Missing parameters");
    }
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

            let wifiResponse = await fetch("/getWiFi");
            let wifi = await wifiResponse.json();
            document.getElementById("ssid").value = wifi.ssid;
            document.getElementById("pass").value = wifi.pass;
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

        async function saveWiFi() {
            let ssid = document.getElementById("ssid").value;
            let pass = document.getElementById("pass").value;
            await fetch(`/setWiFi?ssid=${ssid}&pass=${pass}`);
        }

        function goToUpdate() {
            window.location.href = "/update";
        }

        async function startWiFiConfig() {
            await fetch("/startConfig");
        }

        async function reboot() {
            await fetch("/reboot");
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
    <h3>WiFi Settings</h3>
    <label for="ssid">WiFi SSID:</label>
    <input id="ssid" placeholder="WiFi SSID">
    <label for="pass">WiFi Password:</label>
    <input id="pass" placeholder="WiFi Password">
    <button onclick="saveWiFi()">Save WiFi</button>    
    <br><br>
    <button onclick="goToUpdate()">Update Firmware</button>
    <br><br>
    <button onclick="reboot()">Reboot Device</button>
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

    server.on("/getWiFi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = "{\"ssid\": \"" + wifiSSID + "\", \"pass\": \"" + wifiPass + "\"}";
        request->send(200, "application/json", json);
    });

    server.on("/setWiFi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleWiFiSettings(request);        
    });    

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Rebooting...");
        delay(1000);
        ESP.restart();
    });
}
