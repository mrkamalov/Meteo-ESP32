#ifndef CONST_H
#define CONST_H

// Server details
const char server[] = "mrkamalov.github.io";//https://
const int  port     = 443;//80;
const char resource[]    = "/Meteo-ESP32/firmware/firmware_part";
const char configAddress[] = "/Meteo-ESP32/firmware/config.txt";

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8

/*
Modem test commands
AT
ATE0
AT+CPIN?
AT+CIPSHUT
AT+CGATT=1
AT+SAPBR=3,1,"Contype","GPRS"
AT+CSTT="internet.altel.kz","",""
AT+SAPBR=1,1
AT+SAPBR=2,1
*/
#endif // CONST_H