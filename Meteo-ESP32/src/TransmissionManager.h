#pragma once
#include "MeteoConfigPortal.h"
#include "sim868Config.h"
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include "Sim800Updater.h"
#include "Sim868Client.h"

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
    
    Sim868Client* simClient;
};
