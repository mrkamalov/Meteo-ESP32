#include "MeteoConfigPortal.h"
#include <EEPROM.h>
#include <ESPmDNS.h>
#include "sim868Config.h"

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
    SerialMon.println("Device ID: " + String(deviceId));
    loadWiFiSettings();
    loadDevicesFromEEPROM();
    loadGPRSSettings();
    loadTransferPriority();
    if (connectToWiFi()) {
        SerialMon.println("Connected to WiFi: " + wifiSSID);
        SerialMon.println("IP Address: " + WiFi.localIP().toString());
    }
    else{
        SerialMon.println("Failed to connect to WiFi. Starting AP...");
        startAccessPoint();
    }
    if (MDNS.begin("meteo"+String(deviceId))) {
        SerialMon.println("MDNS responder started: http://meteo"+String(deviceId)+".local");
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
            SerialMon.println("Disconnected AP due to WiFi STA connection");
            SerialMon.println("Connected to WiFi: " + wifiSSID);
            SerialMon.println("IP Address: " + WiFi.localIP().toString());            
        }
    }
    else{
        unsigned long currentMillis = millis();        

        // Цикл точки доступа — выключить/включить, если никого нет
        if (apActive) {
            if(currentMillis - lastApCheck > apCheckInterval){
                lastApCheck = currentMillis;
                SerialMon.println("Checking AP status...");
                if (WiFi.softAPgetStationNum() == 0) {
                    SerialMon.println("Cycling AP: No clients connected");
                    WiFi.softAPdisconnect(true);
                    apActive = false;
                    apRestartPending = true;
                    apRestartTime = currentMillis;
                    lastWifiCheck = currentMillis; // Сброс таймера WiFi проверки
                }
                else{
                    SerialMon.println("AP active: " + String(WiFi.softAPgetStationNum()) + " clients connected");
                }
            }            
        }
        else{
            // Проверка подключения к WiFi
            if (currentMillis - lastWifiCheck > wifiCheckInterval) {
                lastWifiCheck = currentMillis;
                SerialMon.println("Restarting WiFi AP...");
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
    SerialMon.println("Started Access Point");
}


bool MeteoConfigPortal::connectToWiFi() {
    if (wifiSSID.isEmpty()) return false;
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        SerialMon.print(".");
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
    SerialMon.println("Devices count: " + String(count));
    EEPROM.write(addr++, count);
    for (const auto &dev : meteoDevices) {
        EEPROM.write(addr++, dev.id);
        for (char c : dev.name) {
            EEPROM.write(addr++, c);
        }
        EEPROM.write(addr++, '\0');
    }
    if(!EEPROM.commit()) {
        SerialMon.println("Ошибка записи в EEPROM");
    }
    else {
        SerialMon.println("Устройства сохранены в EEPROM");
    }
}

// ==== Загрузка устройств из EEPROM ====
void MeteoConfigPortal::loadDevicesFromEEPROM() {
    meteoDevices.clear();
    int addr = DEVICE_LIST_ADDR;
    int count = EEPROM.read(addr++);
    SerialMon.println("Devices count: " + String(count));
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

void MeteoConfigPortal::saveGPRSSettings(String apn, String user, String pass) {
    SerialMon.println("Saving GPRS settings...");
    SerialMon.println("APN: " + apn);
    SerialMon.println("User: " + user);
    SerialMon.println("Pass: " + pass);
    for (int i = 0; i < 20; i++) EEPROM.write(GPRS_APN_ADDR + i, i < apn.length() ? apn[i] : 0);
    for (int i = 0; i < 20; i++) EEPROM.write(GPRS_USER_ADDR + i, i < user.length() ? user[i] : 0);
    for (int i = 0; i < 20; i++) EEPROM.write(GPRS_PASS_ADDR + i, i < pass.length() ? pass[i] : 0);
    EEPROM.commit();
    gprsAPN = apn;
    gprsUser = user;
    gprsPass = pass;    
}

void MeteoConfigPortal::loadGPRSSettings() {
    char apn[21], user[21], pass[21];
    for (int i = 0; i < 20; i++) apn[i] = EEPROM.read(GPRS_APN_ADDR + i);
    for (int i = 0; i < 20; i++) user[i] = EEPROM.read(GPRS_USER_ADDR + i);
    for (int i = 0; i < 20; i++) pass[i] = EEPROM.read(GPRS_PASS_ADDR + i);
    apn[20] = user[20] = pass[20] = '\0';
    gprsAPN = String(apn);
    gprsUser = String(user);
    gprsPass = String(pass);
}

void MeteoConfigPortal::loadTransferPriority() {
    transferPriority = static_cast<DataPriority>(EEPROM.read(TRANSFER_PRIORITY_ADDR));
    if (transferPriority < PRIORITY_WIFI_ONLY || transferPriority >= PRIORITIES_NUMBER) {
        transferPriority = PRIORITY_WIFI_ONLY; // Значение по умолчанию
    }
}

DataPriority MeteoConfigPortal::getTransferPriority() {
    return transferPriority;
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

            let gprsResponse = await fetch("/getGPRS");
            let gprs = await gprsResponse.json();
            document.getElementById("apn").value = gprs.apn;
            document.getElementById("gprsUser").value = gprs.user;
            document.getElementById("gprsPass").value = gprs.pass;

            let priorityResponse = await fetch("/getPriority");
            let priority = await priorityResponse.text();
            document.getElementById("priority").value = priority;
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

        async function saveGPRS() {
            let apn = document.getElementById("apn").value;
            let user = document.getElementById("gprsUser").value;
            let pass = document.getElementById("gprsPass").value;
            await fetch(`/setGPRS?apn=${apn}&user=${user}&pass=${pass}`);
        }

        async function savePriority() {
            let priority = document.getElementById("priority").value;
            await fetch(`/setPriority?priority=${priority}`);
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
    <h3>GPRS Settings</h3>
    <label for="apn">GPRS APN:</label>
    <input id="apn" placeholder="APN">
    <label for="gprsUser">GPRS User:</label>
    <input id="gprsUser" placeholder="GPRS User">
    <label for="gprsPass">GPRS Password:</label>
    <input id="gprsPass" placeholder="GPRS Password">
    <button onclick="saveGPRS()">Save GPRS</button>
    <br><br>
    <label for="priority">Data Transfer Priority:</label>
    <select id="priority">
        <option value="0">Only WiFi</option>
        <option value="1">Only GPRS</option>
        <option value="2">WiFi first, then GPRS</option>
    </select>
    <button onclick="savePriority()">Save Priority</button>
    <br><br>
    <button onclick="goToUpdate()">Update Firmware</button>
    <br><br>
    <button onclick="reboot()">Reboot Device</button>
</body>
</html>
)rawliteral";

// ==== Обработчик запроса на получение списка устройств ====
void MeteoConfigPortal::handleGetDevices() {
    server.on("/devices", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", getMeteoDevicesList());
    });    
}

// ==== Обработчик добавления устройства ====
void MeteoConfigPortal::handleAddDevice() {
    server.on("/add", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("id") || !request->hasParam("name")) {
            request->send(400, "text/plain", "Ошибка: нужны параметры id и name");
            return;
        }

        int id = request->getParam("id")->value().toInt();
        String name = request->getParam("name")->value();

        meteoDevices.push_back({id, name});
        saveDevicesToEEPROM();
        request->send(200, "text/plain", "Устройство добавлено");
    });    
}

// ==== Обработчик удаления устройства ====
void MeteoConfigPortal::handleRemoveDevice() {
    server.on("/remove", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("id")) {
            request->send(400, "text/plain", "Ошибка: нужен параметр id");
            return;
        }

        int id = request->getParam("id")->value().toInt();
        meteoDevices.erase(std::remove_if(meteoDevices.begin(), meteoDevices.end(),
            [id](MeteoDevice &dev) { return dev.id == id; }), meteoDevices.end());
        saveDevicesToEEPROM();
        request->send(200, "text/plain", "Устройство удалено");
    });
}

// ==== Обработчик установки ID ====
void MeteoConfigPortal::handleSetDeviceId() {
    server.on("/setDeviceId", HTTP_GET, [this](AsyncWebServerRequest *request) {
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
    });
}

// ==== Функция получения текущего ID ====
String MeteoConfigPortal::getDeviceId() {
    return String(deviceId);
}

void MeteoConfigPortal::handleGetDeviceId(){
    server.on("/getDeviceId", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", getDeviceId());
    });
}

void MeteoConfigPortal::handleGetWiFi() {
    server.on("/getWiFi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = "{\"ssid\": \"" + wifiSSID + "\", \"pass\": \"" + wifiPass + "\"}";
        request->send(200, "application/json", json);
    });    
}

void MeteoConfigPortal::handleSetWiFi() {
    server.on("/setWiFi", HTTP_GET, [this](AsyncWebServerRequest *request) {
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
    });
}

void MeteoConfigPortal::handleGetGPRS() {
    server.on("/getGPRS", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String json = "{\"apn\": \"" + gprsAPN + "\", \"user\": \"" + gprsUser + "\", \"pass\": \"" + gprsPass + "\"}";
        request->send(200, "application/json", json);
    });
}

void MeteoConfigPortal::handleSetGPRS() {
    server.on("/setGPRS", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("apn") && request->hasParam("user") && request->hasParam("pass")) {
            String apn = request->getParam("apn")->value();
            String user = request->getParam("user")->value();
            String pass = request->getParam("pass")->value();
            saveGPRSSettings(apn, user, pass);
            request->send(200, "text/plain", "GPRS settings saved. Rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
}

void MeteoConfigPortal::handleReboot() {
    server.on("/reboot", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Rebooting...");
        delay(1000);
        ESP.restart();
    });
}

void MeteoConfigPortal::handleRoot() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });
}

void MeteoConfigPortal::handleGetTransferPriority() {
    server.on("/getPriority", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String((int)transferPriority));
    });
}

void MeteoConfigPortal::handleSetTransferPriority() {
    server.on("/setPriority", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("priority")) {
            int val = request->getParam("priority")->value().toInt();
            if (val >= PRIORITY_WIFI_ONLY && val < PRIORITIES_NUMBER) {
                transferPriority = static_cast<DataPriority>(val);
                EEPROM.write(TRANSFER_PRIORITY_ADDR, (uint8_t)transferPriority);
                EEPROM.commit();
                request->send(200, "text/plain", "Priority saved");
                delay(1000);
                ESP.restart();
                return;
            }
        }
        request->send(400, "text/plain", "Invalid priority");
    });
}

// ==== Настройка веб-сервера ====
void MeteoConfigPortal::setupWebServer() {
    handleRoot();
    handleGetDevices();
    handleAddDevice();
    handleRemoveDevice();
    handleSetDeviceId();
    handleGetDeviceId();

    handleGetWiFi();
    handleSetWiFi();

    handleGetGPRS();
    handleSetGPRS();

    handleReboot();

    handleGetTransferPriority();
    handleSetTransferPriority();    
}
