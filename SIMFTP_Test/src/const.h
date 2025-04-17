#ifndef CONST_H
#define CONST_H

//#include <Arduino.h>

// set GSM PIN, if any
//#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details
const char server[] = "mrkamalov.github.io";//https://
const int  port     = 443;//80;
const char resource[]    = "/Meteo-ESP32/firmware/firmware_part";
const char configAddress[] = "/Meteo-ESP32/firmware/config.txt";

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8

// Размер буфера для расчета CRC32
#define BUFFER_SIZE 1024

// Настройки FTP сервера
const char* FTP_SERVER = "66.220.9.50";
const char* FTP_USER = "mrkamalov001";
const char* FTP_PASS = "PSc2q@UVSn5rayc";
const char* FILE_NAME = "firmware.bin";  // Загружаемый файл
// Расположение файла прошивки в SPIFFS
const char* LOCAL_FILE = "/firmware.bin";

// URL файла конфигурации прошивки
const char* configUrl = "https://mrkamalov.github.io/Meteo-ESP32/firmware/config.txt";
#endif // CONST_H