#include <HardwareSerial.h>
#include "sim800Updater.h"
#include "MeteoConfigPortal.h"

#define MODEM_RX 18
#define MODEM_TX 17

static Sim800Updater simUpdater;
static MeteoConfigPortal meteoPortal;

void setup() {
    // Set console baud rate
    Serial.begin(115200);    
    Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    meteoPortal.begin();
    //simUpdater.init();
}

void loop() {    
    while (true) {
        delay(1000);
        if(simUpdater.checkForUpdates()) simUpdater.updateFirmwareViaGPRS();// Объединить в один метод
    }  
}
