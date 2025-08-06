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
#include "ModbusSensor.h"
#include "PowerMonitor.h"

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
    PowerMonitor powerMonitor;
    static const unsigned long UPDATE_CHECK_INTERVAL_MS = 5 * 60 * 1000; // 5 минут
    static const unsigned long GPRS_UPDATE_CHECK_INTERVAL_MS = 30*1000;//1 * 60 * 1000; // 1 минута TODO: Вернуть на 1 минуту
    unsigned long lastUpdateCheck = 0; // Время последней проверки обновления
    bool updateTimeoutPassed = true; // Флаг для отслеживания таймаута    
    unsigned long gprsLastUpdateCheck = 0; // Время последней проверки обновления для GPRS
    bool gprsUpdateTimeoutPassed = true; // Флаг для отслеживания таймаута GPRS    
    char _mqttServer[64];
    uint16_t _mqttPort;
    char _mqttUser[32];
    char _mqttPass[32];
    char _mqttClientId[32];    
    ModbusSensor meteosensor;
    SensorData sensorData;
    char _apn[21];
    char _gprsUser[21];
    char _gprsPass[21];
    
    void loadMQTTSettingsFromEEPROM();
    void loadGPRSSettingsFromEEPROM();
    String getSensorDataJson();
    bool readSensorData(void);
};
