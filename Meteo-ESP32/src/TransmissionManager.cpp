#include "TransmissionManager.h"
#include <WiFi.h>
#include "SerialMon.h"

//static Sim800Updater simUpdater;

TransmissionManager::TransmissionManager()
    : _modem(SerialAT),
      _mqttClient(_modem),      
      _mqtt(_mqttClient),
      meteosensor(1)
{    
    simClient = new Sim868Client(&_modem, &_mqttClient, &_mqtt);
}

void TransmissionManager::begin() {
    meteoPortal.begin();  // Всегда вызывается

    lastPriority = meteoPortal.getTransferPriority();
    
    loadMQTTSettingsFromEEPROM();
    loadGPRSSettingsFromEEPROM();
    wifiUpdater.begin();
    updater.begin();
    
    if (lastPriority == PRIORITY_GPRS_ONLY || lastPriority == PRIORITY_WIFI_THEN_GPRS) {
        simClient->begin(_mqttServer, _mqttPort, _mqttUser, _mqttPass, _mqttClientId,
                         _apn, _gprsUser, _gprsPass);
    }
    if (lastPriority == PRIORITY_WIFI_ONLY || lastPriority == PRIORITY_WIFI_THEN_GPRS) {
        mqttWifiClient.begin(_mqttServer, _mqttPort, _mqttUser, _mqttPass, _mqttClientId);
    }
    meteosensor.begin();
}

void TransmissionManager::loop() {
    DataPriority currentPriority = meteoPortal.getTransferPriority();

    // Можно отслеживать изменения приоритета, если необходимо
    if (currentPriority != lastPriority) {
        lastPriority = currentPriority;        
    }
    
    bool receiveSensorData = readSensorData(); // Чтение данных датчика
    String sensorJson = getSensorDataJson();
    meteoPortal.loop(sensorJson);

    if((millis() - lastUpdateCheck) > UPDATE_CHECK_INTERVAL_MS) {
        updateTimeoutPassed = true;
        lastUpdateCheck = millis();
    }
    if(!firstUpdateCheckDone) {
        updateTimeoutPassed = true;
        firstUpdateCheckDone = true;
    }
    switch (currentPriority) {
        case PRIORITY_WIFI_ONLY:
            mqttWifiClient.loop(sensorData, false); //receiveSensorData); // Обработка MQTT сообщений mytest TODO: ВКЛЮЧИТЬ В ПРОДАКШЕНЕ!!!!!!!!!!!!!!!!!
            if(WiFi.status() == WL_CONNECTED && updateTimeoutPassed) {
                // Если WiFi подключен, то проверяем обновления
                wifiUpdater.updateFirmware();
                updateTimeoutPassed = false; // Сброс таймаута после обновления                             
            } 
            break;

        case PRIORITY_GPRS_ONLY:
            simClient->loop();
            if(simClient->isModemConnected() && updateTimeoutPassed) {
                // Если модем подключен, то проверяем обновления
                updater.updateFirmwareViaGPRS();
                updateTimeoutPassed = false; // Сброс таймаута после обновления          
            } 
            break;

        case PRIORITY_WIFI_THEN_GPRS:
            if(WiFi.status() == WL_CONNECTED){
                mqttWifiClient.loop(sensorData, receiveSensorData); // Обработка MQTT сообщений
                if(updateTimeoutPassed) {
                    // Если WiFi подключен, то проверяем обновления
                    wifiUpdater.updateFirmware();
                    updateTimeoutPassed = false; // Сброс таймаута после обновления
                }
            }
            else {
                simClient->loop();
                if(simClient->isModemConnected() && updateTimeoutPassed) {
                    // Если модем подключен, то проверяем обновления
                    updater.updateFirmwareViaGPRS();
                    updateTimeoutPassed = false; // Сброс таймаута после обновления
                }                
            }
            break;
    }
    
}

bool TransmissionManager::readSensorData() {
    static unsigned long lastPollTime = 0;

    if ((millis() - lastPollTime) >= 2000) { // Таймаут опроса 2 секунды
        lastPollTime = millis();        
        
        if (meteosensor.readSensorData(sensorData, false)) {
            SerialMon.printf("Gas1: %.2f ppm, PM2.5: %.2f µg/m³, Humidity: %.1f%%\n",
                        sensorData.gas1, sensorData.pm25, sensorData.humidity);
            return true; // Данные успешно считаны
        } else {
            SerialMon.println("Ошибка чтения данных от Modbus-датчика");
        }
    }
    return false; // Данные не считаны
}

void TransmissionManager::loadMQTTSettingsFromEEPROM() {
    int addr = MQTT_EEPROM_START;

    bool isEmpty = true;
    for (int i = 0; i < 10; ++i) {
        if (EEPROM.read(MQTT_EEPROM_START + i) != 0xFF) {
            isEmpty = false;
            break;
        }
    }

    for (int i = 0; i < sizeof(_mqttServer); i++) {
        _mqttServer[i] = EEPROM.read(addr++);
    }
    _mqttServer[sizeof(_mqttServer) - 1] = '\0';

    _mqttPort = EEPROM.read(addr++) << 8;
    _mqttPort |= EEPROM.read(addr++);

    for (int i = 0; i < sizeof(_mqttUser); i++) {
        _mqttUser[i] = EEPROM.read(addr++);
    }
    _mqttUser[sizeof(_mqttUser) - 1] = '\0';

    for (int i = 0; i < sizeof(_mqttPass); i++) {
        _mqttPass[i] = EEPROM.read(addr++);
    }
    _mqttPass[sizeof(_mqttPass) - 1] = '\0';

    for (int i = 0; i < sizeof(_mqttClientId); i++) {
        _mqttClientId[i] = EEPROM.read(addr++);
    }
    _mqttClientId[sizeof(_mqttClientId) - 1] = '\0';

    if (isEmpty || _mqttServer[0] == '\0' || _mqttPort == 0 || _mqttUser[0] == '\0' || _mqttPass[0] == '\0' 
        || _mqttClientId[0] == '\0') {
        SerialMon.println("EEPROM is empty, loading default MQTT config");

        strncpy(_mqttServer, MQTT_BROKER_DEFAULT, sizeof(_mqttServer));
        _mqttPort = MQTT_PORT_DEFAULT;
        strncpy(_mqttUser, MQTT_USERNAME_DEFAULT, sizeof(_mqttUser));
        strncpy(_mqttPass, MQTT_PASSWORD_DEFAULT, sizeof(_mqttPass));
        strncpy(_mqttClientId, MQTT_CLIENT_ID_DEFAULT, sizeof(_mqttClientId));
    }
    else SerialMon.println("Loaded MQTT config from EEPROM");
}

void TransmissionManager::loadGPRSSettingsFromEEPROM() {
    int addr = GPRS_APN_ADDR;
    
    // APN (макс 20 байта)
    for (int i = 0; i < 20; ++i) {
        _apn[i] = EEPROM.read(addr++);
    }
    _apn[20] = '\0';

    // GPRS User (макс 20 байта)
    for (int i = 0; i < 20; ++i) {
        _gprsUser[i] = EEPROM.read(addr++);
    }
    _gprsUser[20] = '\0';

    // GPRS Pass (макс 20 байта)
    for (int i = 0; i < 20; ++i) {
        _gprsPass[i] = EEPROM.read(addr++);
    }
    _gprsPass[20] = '\0';

    SerialMon.printf("GPRS Settings: APN=%s, User=%s, Pass=%s\n", _apn, _gprsUser, _gprsPass);
}

String TransmissionManager::getSensorDataJson() {
    const SensorData& d = sensorData;

    String json = "{";
    json += "\"gas1\":" + String(d.gas1, 2) + ",";
    json += "\"gas2\":" + String(d.gas2, 2) + ",";
    json += "\"gas3\":" + String(d.gas3, 2) + ",";
    json += "\"gas4\":" + String(d.gas4, 2) + ",";
    json += "\"internalTemp\":" + String(d.internalTemp, 2) + ",";
    json += "\"pm25\":" + String(d.pm25, 2) + ",";
    json += "\"pm10\":" + String(d.pm10, 2) + ",";
    json += "\"externalTemp\":" + String(d.externalTemp, 2) + ",";
    json += "\"humidity\":" + String(d.humidity, 2);
    json += "}";

    return json;
}