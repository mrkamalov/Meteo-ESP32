#include "TransmissionManager.h"
#include <WiFi.h>

//static Sim800Updater simUpdater;

TransmissionManager::TransmissionManager()
    : _modem(SerialAT),
      _mqttClient(_modem),      
      _mqtt(_mqttClient)      
{    
    simClient = new Sim868Client(&_modem, &_mqttClient, &_mqtt);
}

void TransmissionManager::begin() {
    meteoPortal.begin();  // Всегда вызывается

    lastPriority = meteoPortal.getTransferPriority();
    
    loadSettingsFromEEPROM();
    wifiUpdater.begin();
    updater.begin();
    
    if (lastPriority == PRIORITY_GPRS_ONLY || lastPriority == PRIORITY_WIFI_THEN_GPRS) {
        simClient->begin(_mqttServer, _mqttPort, _mqttUser, _mqttPass, _mqttClientId);
    }
    if (lastPriority == PRIORITY_WIFI_ONLY || lastPriority == PRIORITY_WIFI_THEN_GPRS) {
        mqttWifiClient.begin(_mqttServer, _mqttPort, _mqttUser, _mqttPass, _mqttClientId);
    }
}

void TransmissionManager::loop() {
    DataPriority currentPriority = meteoPortal.getTransferPriority();

    // Можно отслеживать изменения приоритета, если необходимо
    if (currentPriority != lastPriority) {
        lastPriority = currentPriority;
        // (Опционально) Реинициализация компонентов при изменении приоритета
    }

    // WiFi используется в любом случае для конфигурации
    meteoPortal.loop();
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
            mqttWifiClient.loop(); // Обработка MQTT сообщений
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
                mqttWifiClient.loop(); // Обработка MQTT сообщений
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

void TransmissionManager::loadSettingsFromEEPROM() {
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

        strncpy(_mqttServer, MQTT_BROKER, sizeof(_mqttServer));
        _mqttPort = MQTT_PORT;
        strncpy(_mqttUser, MQTT_USERNAME, sizeof(_mqttUser));
        strncpy(_mqttPass, MQTT_PASSWORD, sizeof(_mqttPass));
        strncpy(_mqttClientId, MQTT_CLIENT_ID, sizeof(_mqttClientId));
    }
    else SerialMon.println("Loaded MQTT config from EEPROM");    
}