#ifndef CONST_H
#define CONST_H

#include <Arduino.h>

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// Server details
const char server[] = "mrkamalov.github.io";//https://
const int  port     = 443;//80;
const char resource[]    = "/Meteo-ESP32/test50k.bin";

#endif // CONST_H