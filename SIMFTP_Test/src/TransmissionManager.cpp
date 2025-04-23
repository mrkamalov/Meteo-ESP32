#include "TransmissionManager.h"
#include <WiFi.h>

//static Sim800Updater simUpdater;

TransmissionManager::TransmissionManager(){}

void TransmissionManager::begin() {
    meteoPortal.begin();  // Всегда вызывается

    lastPriority = meteoPortal.getTransferPriority();

    if (lastPriority == PRIORITY_GPRS_ONLY || lastPriority == PRIORITY_WIFI_THEN_GPRS) {
        simClient.begin();
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

    switch (currentPriority) {
        case PRIORITY_WIFI_ONLY:
            // Только WiFi — никаких действий с GPRS
            break;

        case PRIORITY_GPRS_ONLY:
            simClient.loop();
            break;

        case PRIORITY_WIFI_THEN_GPRS:
            if (WiFi.status() != WL_CONNECTED) {
                simClient.loop();
            }
            break;
    }
}
