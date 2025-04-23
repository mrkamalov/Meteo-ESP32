#pragma once
#include "MeteoConfigPortal.h"
#include "Sim868Client.h"

class TransmissionManager {
public:
    TransmissionManager();

    void begin();
    void loop();

private:
    MeteoConfigPortal meteoPortal;
    Sim868Client simClient;
    DataPriority lastPriority = PRIORITY_WIFI_ONLY; // Для отслеживания изменений
};
