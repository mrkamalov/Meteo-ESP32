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

    if (lastPriority == PRIORITY_GPRS_ONLY || lastPriority == PRIORITY_WIFI_THEN_GPRS) {
        simClient->begin();
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
            if(WiFi.status() == WL_CONNECTED && updateTimeoutPassed) {
                // Если WiFi подключен, то проверяем обновления
                wifiUpdater.updateFirmware();
                updateTimeoutPassed = false; // Сброс таймаута после обновления
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
