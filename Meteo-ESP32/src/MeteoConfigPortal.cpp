#include "MeteoConfigPortal.h"
#include <EEPROM.h>
#include <ESPmDNS.h>
#include "deviceConfig.h"
#include "SerialMon.h"

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
    SerialMon.println("WiFi SSID: " + wifiSSID);    
    SerialMon.println("WiFi Pass: " + wifiPass);    
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

void MeteoConfigPortal::loop(const String& sensorJson) {
    _cachedSensorJson = sensorJson;
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
    String output = "";
    for (const auto &dev : meteoDevices) {
        output += "Modbus ID: " + String(dev.id) + ", Название: " + dev.name + "\n";
    }
    return output;
}

std::vector<MeteoDevice> MeteoConfigPortal::getMeteoDevices() {
  return meteoDevices;
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

void MeteoConfigPortal::saveMQTTToEEPROM(const String& broker, uint16_t port, const String& user, const String& pass, const String& clientId) {
    int addr = MQTT_EEPROM_START;

    // Broker (макс 64 байта)
    for (int i = 0; i < 64; ++i) {
        char c = i < broker.length() ? broker[i] : 0;
        EEPROM.write(addr++, c);
    }
    
    // Port (2 байта)
    EEPROM.write(addr++, (port >> 8) & 0xFF);  // старший байт
    EEPROM.write(addr++, port & 0xFF);         // младший байт

    // Username (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        char c = i < user.length() ? user[i] : 0;
        EEPROM.write(addr++, c);
    }

    // Password (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        char c = i < pass.length() ? pass[i] : 0;
        EEPROM.write(addr++, c);
    }

    // Client ID (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        char c = i < clientId.length() ? clientId[i] : 0;
        EEPROM.write(addr++, c);
    }

    EEPROM.commit(); 
    SerialMon.println("MQTT settings saved to EEPROM:");
    SerialMon.printf("Broker: %s, Port: %d, User: %s, Pass: %s, Client ID: %s\n", 
        broker.c_str(), port, user.c_str(), pass.c_str(), clientId.c_str()); 
}

void MeteoConfigPortal::loadMQTTFromEEPROM(char* broker, uint16_t& port, char* user, char* pass, char* clientId) {
    int addr = MQTT_EEPROM_START;

    // Broker (макс 64 байта)
    for (int i = 0; i < 64; ++i) {
        broker[i] = EEPROM.read(addr++);
    }
    broker[63] = '\0';

    // Port (2 байта)
    port = EEPROM.read(addr++) << 8;
    port |= EEPROM.read(addr++);

    // Username (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        user[i] = EEPROM.read(addr++);
    }
    user[31] = '\0';

    // Password (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        pass[i] = EEPROM.read(addr++);
    }
    pass[31] = '\0';

    // Client ID (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        clientId[i] = EEPROM.read(addr++);
    }
    clientId[31] = '\0';
}

void MeteoConfigPortal::saveFTPToEEPROM(const String& server, const String& user, const String& pass) {
    int addr = FTP_EEPROM_ADDR;

    // Server (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        char c = i < server.length() ? server[i] : 0;
        EEPROM.write(addr++, c);
    }

    // Username (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        char c = i < user.length() ? user[i] : 0;
        EEPROM.write(addr++, c);
    }

    // Password (макс 32 байта)
    for (int i = 0; i < 32; ++i) {
        char c = i < pass.length() ? pass[i] : 0;
        EEPROM.write(addr++, c);
    }

    EEPROM.commit();
    SerialMon.println("FTP settings saved to EEPROM:");
    SerialMon.printf("Server: %s, User: %s, Pass: %s\n", server.c_str(), user.c_str(), pass.c_str());
}

void MeteoConfigPortal::saveHttpServerToEEPROM(const String& server) {
    int addr = HTTP_SERVER_EEPROM_ADDR;

    // Server (макс 64 байта)
    for (int i = 0; i < 64; ++i) {
        char c = i < server.length() ? server[i] : 0;
        EEPROM.write(addr++, c);
    }

    EEPROM.commit();
    SerialMon.println("HTTP Server settings saved to EEPROM:");
    SerialMon.printf("Server: %s\n", server.c_str());
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <title>Управление устройством</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">																		
  <style>
  body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
    background: #f4f4f4;
    color: #333;
  }

  header {
    background: #013d85;
    color: white;
    padding: 15px;
    text-align: center;
  }

  nav {
    background: #333;
    display: flex;
    flex-wrap: wrap;
    justify-content: center;
  }

  nav a {
    color: white;
    padding: 10px 15px;
    text-decoration: none;
    display: block;
    text-align: center;
  }

  nav a:hover {
    background: #1c3341;
  }

  section {
    display: none;
    padding: 20px;
  }

  section.active {
    display: block;
  }

  h2 {
    margin-top: 0;
  }

  .card {
    background: #fff;
    padding: 20px;
    margin: 10px auto;
    border-radius: 8px;
    box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
    max-width: 500px; /* ограничиваем ширину карточки */
  }

  .card input,
  .card select,
  .card button {
    display: block;
    width: 100%;
    padding: 10px;
    margin: 8px 0;
    box-sizing: border-box;
    border: 1px solid #ccc;
    border-radius: 6px;
    font-size: 14px;
  }

  .card button {
    background: #013d85;
    color: white;
    border: none;
    cursor: pointer;
    margin: 10px auto; /* центрируем */
  }

  .card button:hover {
    background: #012c60;
  }

  table {
    border-collapse: collapse;
    width: 100%;
    font-size: 14px;
  }

  th,
  td {
    border: 1px solid #ccc;
    padding: 6px 10px;
    text-align: left;
  }

  th {
    background: #f0f0f0;
  }

  /* Контейнер для форм */
  .form-row {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin: 10px 0;
  }

  .form-row > * {
    flex: 1 1 200px;
  }

  /* Адаптивные стили */
  @media (max-width: 600px) {
    .card {
      max-width: 100%;
    }
  }
  </style>
</head>
<body>
  <header>
    <h1>Sergek Eco SE-1</h1>
  </header>
  <nav>
    <a href="#" onclick="showSection('sensor_values')">Мониторинг</a>
    <a href="#" onclick="showSection('calibration')">Калибровка</a>
    <a href="#" onclick="showSection('sensors_settings')">Список устройств</a>
    <a href="#" onclick="showSection('wifi_gprs')">Настройки WiFi/GPRS</a>
    <a href="#" onclick="showSection('mqtt')">Настройки MQTT</a>
    <a href="#" onclick="showSection('ftp_http')">Настройки FTP/HTTP сервера</a>
    <a href="#" onclick="showSection('system')">Системные настройки</a>
  </nav>

  <section id="sensor_values" class="active">
    <div class="card">
      <h2>Мониторинг</h2>
      <table>
        <thead>
          <tr>
            <th>Параметр</th>
            <th>Значение</th>
          </tr>
        </thead>
        <tbody id="sensorData">
          <tr><td colspan="2">Загрузка...</td></tr>
        </tbody>
      </table>
    </div>
  </section>

  <section id="calibration">
    <div class="card">
      <h2>Калибровка</h2>      
    </div>
  </section>

  <section id="sensors_settings">
    <div class="card">
      <h2>Список устройств</h2>
      <pre id="deviceList"></pre>
	  <div class="form-row">					
      <input id="id" placeholder="Modbus ID">
      <select id="name">
      <option value="SE-1">SE-1</option>      
      </select>
	  </div>
    <div class="form-row">
      <button onclick="addDevice()">Добавить</button>
      <button onclick="removeDevice()">Удалить</button>
    </div>
      <button onclick="reboot()">Перезапустить опрос</button>
    </div>
  </section>

  <section id="wifi_gprs">
    <div class="card">
      <h2>WiFi / GPRS / Приоритет передачи</h2>
	  <div class="form-row">
      <label for="ssid">WiFi SSID:</label>
      <input id="ssid" placeholder="WiFi SSID">
      <label for="pass">WiFi Password:</label>
      <input id="pass" placeholder="WiFi Password">	  
      <label for="apn">GPRS APN:</label>
      <input id="apn" placeholder="APN">
      <label for="gprsUser">GPRS User:</label>
      <input id="gprsUser" placeholder="GPRS User">
      <label for="gprsPass">GPRS Password:</label>
      <input id="gprsPass" placeholder="GPRS Password">	  
      <label for="priority">Data Transfer Priority:</label>
      <select id="priority">
          <option value="0">Only WiFi</option>
          <option value="1">Only GPRS</option>
          <option value="2">WiFi first, then GPRS</option>
      </select>
	  </div>	
      <button onclick="saveNetworkSettings()">Сохранить настройки</button>
    </div>
  </section>    

  <section id="mqtt">
    <div class="card">
      <h2>Настройки MQTT</h2>
      <div class="form-row">
        <label for="mqttBroker">MQTT Broker:</label>
        <input id="mqttBroker" placeholder="Broker">
        <label for="mqttPort">MQTT Port:</label>
        <input id="mqttPort" type="number" placeholder="Port">        
        <label for="mqttUser">MQTT Username:</label>
        <input id="mqttUser" placeholder="Username">
        <label for="mqttClientId">MQTT Client ID:</label>
        <input id="mqttClientId" placeholder="Client ID">
        <label for="mqttPass">MQTT Password:</label>
        <input id="mqttPass" placeholder="Password">
        <button onclick="saveMQTT()">Сохранить настройки</button>
      </div>
    </div>
  </section>

  <section id="ftp_http">
    <div class="card">
      <h2>Настройки FTP/HTTP сервера</h2>
      <div class="form-row">
        <label for="ftpServer">FTP Server:</label>
        <input id="ftpServer" placeholder="FTP Server">
        <label for="ftpUser">FTP User:</label>
        <input id="ftpUser" placeholder="FTP User">
        <label for="ftpPass">FTP Password:</label>
        <input id="ftpPass" placeholder="FTP Password">
        <label for="httpServer">HTTP Server URL:</label>
        <input id="httpServer" placeholder="https://example.com/firmware">
        <button onclick="saveServers()">Сохранить настройки</button>
      </div>
    </div>
  </section>

  <section id="system">
    <div class="card">
      <h2>Системные настройки</h2>
      <div class="form-row">
      <label for="deviceId">Серийный номер устройства:</label>
      <input id="deviceId" placeholder="Серийный номер устройства">
      <button onclick="setDeviceId()">Сохранить</button>
      </div><br><br>
      <button onclick="goToUpdate()">Обновление ПО</button>      
      <button onclick="setDefaults()">Установить настройки по умолчанию</button>
      <button onclick="reboot()">Перезагрузить устройство</button>
    </div>
  </section>

  <script>
    async function saveNetworkSettings() {
      let ssid     = encodeURIComponent(document.getElementById("ssid").value);
      let pass     = encodeURIComponent(document.getElementById("pass").value);
      let apn      = encodeURIComponent(document.getElementById("apn").value);
      let user     = encodeURIComponent(document.getElementById("gprsUser").value);
      let gprsPass = encodeURIComponent(document.getElementById("gprsPass").value);
      let priority = encodeURIComponent(document.getElementById("priority").value);

      let url = `/saveNetworkSettings?ssid=${ssid}&pass=${pass}&apn=${apn}&user=${user}&gprsPass=${gprsPass}&priority=${priority}`;
      let response = await fetch(url);
    }

    async function loadDeviceSection() {
      // Получение списка устройств
      let response = await fetch("/devices");
      let data = await response.text();
      document.getElementById("deviceList").innerText = data;

      // Получение Device ID
      let deviceIdResponse = await fetch("/getDeviceId");
      let deviceId = await deviceIdResponse.text();
      document.getElementById("deviceId").value = deviceId;
    }

    async function loadNetworkSettings() {    
      // WiFi
      let wifiResponse = await fetch("/getWiFi");
      let wifi = await wifiResponse.json();
      document.getElementById("ssid").value = wifi.ssid;
      document.getElementById("pass").value = wifi.pass;

      // GPRS
      let gprsResponse = await fetch("/getGPRS");
      let gprs = await gprsResponse.json();
      document.getElementById("apn").value = gprs.apn;
      document.getElementById("gprsUser").value = gprs.user;
      document.getElementById("gprsPass").value = gprs.pass;

      // Priority
      let priorityResponse = await fetch("/getPriority");
      let priority = await priorityResponse.text();
      document.getElementById("priority").value = priority;    
    }

    async function saveMQTT() {
      let broker = encodeURIComponent(document.getElementById("mqttBroker").value);
      let port = document.getElementById("mqttPort").value;
      let user = encodeURIComponent(document.getElementById("mqttUser").value);
      let pass = encodeURIComponent(document.getElementById("mqttPass").value);
      let clientId = encodeURIComponent(document.getElementById("mqttClientId").value);

      await fetch(`/setMQTT?broker=${broker}&port=${port}&user=${user}&pass=${pass}&clientId=${clientId}`);
    }

    async function loadMQTT() {
      const response = await fetch("/getMQTT");
      const mqtt = await response.json();
      document.getElementById("mqttBroker").value = mqtt.broker;
      document.getElementById("mqttPort").value = mqtt.port;
      document.getElementById("mqttUser").value = mqtt.user;
      document.getElementById("mqttPass").value = mqtt.pass;
      document.getElementById("mqttClientId").value = mqtt.clientId;
    }

    async function loadFTP() {
      let ftpResponse = await fetch("/getFTP");
      let ftp = await ftpResponse.json();
      document.getElementById("ftpServer").value = ftp.server;
      document.getElementById("ftpUser").value = ftp.user;
      document.getElementById("ftpPass").value = ftp.pass;
    }

    async function loadHttpServer() {
      let response = await fetch("/getHttpServer");
      let server = await response.text();
      document.getElementById("httpServer").value = server;
    }

    async function saveServers() {
      let ftpServer = encodeURIComponent(document.getElementById("ftpServer").value);
      let ftpUser   = encodeURIComponent(document.getElementById("ftpUser").value);
      let ftpPass   = encodeURIComponent(document.getElementById("ftpPass").value);
      let httpServer = encodeURIComponent(document.getElementById("httpServer").value);

      let url = `/setServers?ftpServer=${ftpServer}&ftpUser=${ftpUser}&ftpPass=${ftpPass}&httpServer=${httpServer}`;
      let response = await fetch(url);    
    }

    async function setDefaults() {
      if (confirm("Вы уверены, что хотите установить настройки по умолчанию?")) {
        await fetch("/setDefaults");
        alert("Настройки по умолчанию установлены. Устройство будет перезагружено");
      }
    }

    async function reboot() {
      await fetch("/reboot");
    }

    async function goToUpdate() {
      window.location.href = "/update";
    }

    async function addDevice() {
      let id = document.getElementById("id").value;
      let name = document.getElementById("name").value;
      let response = await fetch(`/add?id=${id}&name=${name}`);

      if (!response.ok) {        
        alert(await response.text());
        return;
      }

      loadDeviceSection();
    }

    async function removeDevice() {
      let id = document.getElementById("id").value;
      await fetch(`/remove?id=${id}`);
      loadDeviceSection();
    }

    async function setDeviceId() {
      let deviceId = document.getElementById("deviceId").value;
      await fetch(`/setDeviceId?deviceId=${deviceId}`);
    }

    function showSection(id) {
      document.querySelectorAll("section").forEach(sec => sec.classList.remove("active"));
      document.getElementById(id).classList.add("active");
    }

    // AJAX: загрузка данных сенсоров каждые 3 секунды
    function loadSensorData() {
      fetch('/getSensorData')
        .then(response => response.json())
        .then(data => {
          let rows = "";
          for (const [key, value] of Object.entries(data)) {
            rows += `<tr><td>${key}</td><td>${Number(value).toFixed(2)}</td></tr>`;
          }
          document.getElementById('sensorData').innerHTML = rows;
        })
        .catch(err => {
          document.getElementById('sensorData').innerHTML =
            "<tr><td colspan='2'>Ошибка загрузки</td></tr>";
        });
    }

    setInterval(() => {
      if (document.getElementById('sensor_values').classList.contains('active')) {
        loadSensorData();
      }
    }, 3000);

    window.onload = function() {
      loadSensorData();
      loadDeviceSection();
      loadNetworkSettings();
      loadMQTT();
      loadFTP();
      loadHttpServer();
    };    
  </script>
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
        if(id <= 0 || id > 254) {
            request->send(400, "text/plain", "Ошибка: некорректный ID устройства");
            return;
        }

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
        if(id < 0 || id > 254) {
            request->send(400, "text/plain", "Ошибка: некорректный ID устройства");
            return;
        }
        saveDeviceId(id);
        request->send(200, "text/plain", "ID устройства сохранен");
        delay(1000);
        ESP.restart();
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

void MeteoConfigPortal::handleSaveNetworkSettings() {
    server.on("/saveNetworkSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
      bool handled = false;

      // ---- WiFi ----
      if (request->hasParam("ssid") && request->hasParam("pass")) {
        String ssid = request->getParam("ssid")->value();
        String pass = request->getParam("pass")->value();
        saveWiFiSettings(ssid, pass);
        handled = true;
      }
      // ---- GPRS ----
      if (request->hasParam("apn") && request->hasParam("user") && request->hasParam("gprsPass")) {
        String apn  = request->getParam("apn")->value();
        String user = request->getParam("user")->value();
        String gprsPass = request->getParam("gprsPass")->value();
        saveGPRSSettings(apn, user, gprsPass);
        handled = true;
      }
      // ---- Priority ----
      if (request->hasParam("priority")) {
        int val = request->getParam("priority")->value().toInt();
        if (val >= PRIORITY_WIFI_ONLY && val < PRIORITIES_NUMBER) {
          transferPriority = static_cast<DataPriority>(val);
          EEPROM.write(TRANSFER_PRIORITY_ADDR, (uint8_t)transferPriority);
          EEPROM.commit();
          handled = true;
        }
      }
      if (handled) {
        request->send(200, "text/plain", "Settings saved. Rebooting...");
        delay(1000);
        ESP.restart();
      } else {
        request->send(400, "text/plain", "Missing or invalid parameters");
      }
    });
}

void MeteoConfigPortal::handleGetGPRS() {
    server.on("/getGPRS", HTTP_GET, [this](AsyncWebServerRequest *request) {
      String json = "{\"apn\": \"" + gprsAPN + "\", \"user\": \"" + gprsUser + "\", \"pass\": \"" + gprsPass + "\"}";
      request->send(200, "application/json", json);
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

void MeteoConfigPortal::handleGetMQTT() {
    server.on("/getMQTT", HTTP_GET, [this](AsyncWebServerRequest *request) {
      char broker[64], user[32], pass[32], clientId[32];
      uint16_t port;
      loadMQTTFromEEPROM(broker, port, user, pass, clientId);
      String response = "{";
      response += "\"broker\":\"" + String(broker) + "\",";
      response += "\"port\":" + String(port) + ",";
      response += "\"user\":\"" + String(user) + "\",";
      response += "\"pass\":\"" + String(pass) + "\",";
      response += "\"clientId\":\"" + String(clientId) + "\"";
      response += "}";
      request->send(200, "application/json", response);
    });
}

void MeteoConfigPortal::handleSetMQTT() {
    server.on("/setMQTT", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (request->hasParam("broker") && request->hasParam("port") &&
        request->hasParam("user") && request->hasParam("pass") &&
        request->hasParam("clientId")) {
        
        String broker = request->getParam("broker")->value();
        uint16_t port = request->getParam("port")->value().toInt();
        String user = request->getParam("user")->value();
        String pass = request->getParam("pass")->value();
        String clientId = request->getParam("clientId")->value();
        saveMQTTToEEPROM(broker, port, user, pass, clientId);
        request->send(200, "text/plain", "MQTT settings saved");
        delay(1000);
        ESP.restart();
      } else {
        request->send(400, "text/plain", "Missing parameters");
      }
    });
}

void MeteoConfigPortal::handleGetFTP() {
    server.on("/getFTP", HTTP_GET, [this](AsyncWebServerRequest *request) {
      char ftpServer[32], ftpUser[32], ftpPass[32];
      int addr = FTP_EEPROM_ADDR;

      for (int i = 0; i < sizeof(ftpServer); i++) ftpServer[i] = EEPROM.read(addr++);
      ftpServer[31] = '\0';  // Завершающий нуль-символ
      for (int i = 0; i < sizeof(ftpUser); i++) ftpUser[i] = EEPROM.read(addr++);
      ftpUser[31] = '\0';  // Завершающий нуль-символ
      for (int i = 0; i < sizeof(ftpPass); i++) ftpPass[i] = EEPROM.read(addr++);
      ftpPass[31] = '\0';  // Завершающий нуль-символ

      String json = "{";
      json += "\"server\":\"" + String(ftpServer) + "\",";
      json += "\"user\":\"" + String(ftpUser) + "\",";
      json += "\"pass\":\"" + String(ftpPass) + "\"";
      json += "}";

      request->send(200, "application/json", json);
    });
}

void MeteoConfigPortal::handleSetServers() {
  server.on("/setServers", HTTP_GET, [this](AsyncWebServerRequest *request) {
    bool updated = false;

    // HTTP сервер
    if (request->hasParam("httpServer")) {
      String serverVal = request->getParam("httpServer")->value();
      int addr = HTTP_SERVER_EEPROM_ADDR;

      for (int i = 0; i < 64; i++) {
        EEPROM.write(addr++, i < serverVal.length() ? serverVal[i] : 0);
      }
      updated = true;
    }

    // FTP сервер + логин + пароль
    if (request->hasParam("ftpServer") && request->hasParam("ftpUser") && request->hasParam("ftpPass")) {
      String serverVal = request->getParam("ftpServer")->value();
      String userVal   = request->getParam("ftpUser")->value();
      String passVal   = request->getParam("ftpPass")->value();

      int addr = FTP_EEPROM_ADDR;

      for (int i = 0; i < 32; i++)
        EEPROM.write(addr++, i < serverVal.length() ? serverVal[i] : 0);
      for (int i = 0; i < 32; i++)
        EEPROM.write(addr++, i < userVal.length() ? userVal[i] : 0);
      for (int i = 0; i < 32; i++)
        EEPROM.write(addr++, i < passVal.length() ? passVal[i] : 0);

      updated = true;
    }

    if (updated) {
      EEPROM.commit();
      request->send(200, "text/plain", "Settings saved");
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });
}

void MeteoConfigPortal::handleGetHttpServer() {
  server.on("/getHttpServer", HTTP_GET, [](AsyncWebServerRequest *request) {
    char server[64];
    int addr = HTTP_SERVER_EEPROM_ADDR;

    for (int i = 0; i < 64; i++) {
        server[i] = EEPROM.read(addr++);
    }
    server[63] = '\0';

    request->send(200, "text/plain", String(server));
  });
}

void MeteoConfigPortal::handleGetSensorData() {
  server.on("/getSensorData", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (_cachedSensorJson.isEmpty()) {
      request->send(200, "application/json", "{\"Загрузка...\":\"Нет данных\"}");
    } else {
      request->send(200, "application/json", _cachedSensorJson);
    }
  });
}

void MeteoConfigPortal::handleSetDefaults() {
  server.on("/setDefaults", HTTP_GET, [this](AsyncWebServerRequest *request) {        
    for (int i = 10; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();

    // Сброс всех настроек к значениям по умолчанию
    saveWiFiSettings(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
    saveGPRSSettings(APN_DEFAULT, GPRS_USER_DEFAULT, GPRS_PASS_DEFAULT);
    saveMQTTToEEPROM(MQTT_BROKER_DEFAULT, MQTT_PORT_DEFAULT, MQTT_USERNAME_DEFAULT, MQTT_PASSWORD_DEFAULT, MQTT_CLIENT_ID_DEFAULT);
    saveFTPToEEPROM(FTP_SERVER_DEFAULT, FTP_USER_DEFAULT, FTP_PASS_DEFAULT);
    saveHttpServerToEEPROM(HTTP_SERVER_DEFAULT);
    saveDeviceId(0);
    transferPriority = PRIORITY_WIFI_ONLY;
    EEPROM.write(TRANSFER_PRIORITY_ADDR, (uint8_t)transferPriority);
    EEPROM.commit();
    
    // Сброс списка устройств
    meteoDevices.clear();
    saveDevicesToEEPROM();

    request->send(200, "text/plain", "Default settings applied. Rebooting...");
    delay(1000);
    ESP.restart();
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

    handleSaveNetworkSettings();
    handleGetWiFi();    
    handleGetGPRS();    
    handleGetTransferPriority();  
    handleReboot();    

    handleGetMQTT();
    handleSetMQTT();

    handleSetServers();
    handleGetFTP();
    handleGetHttpServer();
    
    handleGetSensorData();    

    handleSetDefaults();
}
