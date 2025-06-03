#pragma once
#include "MeteoConfigPortal.h"
#include "deviceConfig.h"
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include "Sim800Updater.h"
#include "Sim868Client.h"
#include "WiFiUpdater.h"
#include "ServerSettings.h"
#include "MqttWifiClient.h"

class TransmissionManager {
public:
    TransmissionManager();

    void begin();
    void loop();

private:
    MeteoConfigPortal meteoPortal;
    Sim800Updater updater;  
    DataPriority lastPriority = PRIORITY_WIFI_ONLY; // Для отслеживания изменений
    TinyGsm _modem;
    TinyGsmClient _mqttClient;    
    PubSubClient _mqtt;
    WiFiUpdater wifiUpdater;
    Sim868Client* simClient;
    MqttWifiClient mqttWifiClient;
    static const unsigned long UPDATE_CHECK_INTERVAL_MS = 5 * 60 * 1000; // 5 минут
    unsigned long lastUpdateCheck = 0; // Время последней проверки обновления
    bool updateTimeoutPassed = false; // Флаг для отслеживания таймаута
    bool firstUpdateCheckDone = false;
    char _mqttServer[64];
    uint16_t _mqttPort;
    char _mqttUser[32];
    char _mqttPass[32];
    char _mqttClientId[32];
    void loadSettingsFromEEPROM();
};
