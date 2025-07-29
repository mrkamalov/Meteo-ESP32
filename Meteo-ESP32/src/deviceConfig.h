#pragma once

#include "SerialMon.h"

#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_GSM_EN_PIN 19
#define MODEM_PWRKEY_PIN 37
#define MODEM_STATUS_PIN 38
#define SMS_TARGET  "+77022384851"
// Select your modem:
#define TINY_GSM_MODEM_SIM800
// Set serial for debug console (to the Serial Monitor, default speed 115200)
//#define SerialMon Serial
#define SerialAT Serial1
// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// set GSM PIN, if any
#define GSM_PIN ""
#define LED_PIN 1
// WIFI
#define WIFI_SSID_DEFAULT   "SG_RND"
#define WIFI_PASS_DEFAULT   "korkemwifi"
// GPRS
#define APN_DEFAULT         "internet.altel.kz"
#define GPRS_USER_DEFAULT   ""
#define GPRS_PASS_DEFAULT   ""
// MQTT
#define MQTT_USERNAME_DEFAULT "CjQvGwkeDxwLOC8FKCMcACA"
#define MQTT_CLIENT_ID_DEFAULT "CjQvGwkeDxwLOC8FKCMcACA"
#define MQTT_PASSWORD_DEFAULT "VUgJOQcuoIl+A4OQXmr4Gs01"
#define MQTT_BROKER_DEFAULT   "mqtt3.thingspeak.com"
#define MQTT_PORT_DEFAULT     1883
#define MQTT_CHANNEL_ID 2385945
#define SUBSCRIBE_TO_CHANNEL 0

// Настройки FTP сервера
#define FTP_SERVER_DEFAULT "ftpcouch.couchdrop.io"
#define FTP_USER_DEFAULT "couchuser"
#define FTP_PASS_DEFAULT "lqKJ4MAUy8g0g06IB8rn"

#define HTTP_SERVER_DEFAULT "https://mrkamalov.github.io/Meteo-ESP32/firmware" //"172.30.56.14";

// ==== Аппаратные настройки Modbus ====
#define MODBUS_BAUDRATE   9600
#define MODBUS_RX_PIN     41
#define MODBUS_TX_PIN     42
#define MODBUS_REDE_PIN   8

// Пин для мониторинга питания
#define EXTERNAL_POWER_PIN 16
#define POWER_LOSS_THRESHOLD 1500  // мВ (ниже считаем что питание пропало)
#define SET_DEFAULTS_PIN 48

struct SensorData {
    float gas1;
    float gas2;
    float gas3;
    float gas4;
    float internalTemp;
    float pm25;
    float pm10;
    float externalTemp;
    float humidity;
};

constexpr int EEPROM_SIZE = 1024;    
constexpr int WIFI_SSID_ADDR = 10;
constexpr int WIFI_PASS_ADDR = 30;
constexpr int GPRS_APN_ADDR = 50;
constexpr int GPRS_USER_ADDR = 70;
constexpr int GPRS_PASS_ADDR = 90;
constexpr int TRANSFER_PRIORITY_ADDR  = 110;
constexpr int FW_VERSION_ADDR  = 114;
constexpr int FW_VERSION_SIZE = 8;
constexpr int MQTT_EEPROM_START = 122;
constexpr int FTP_EEPROM_ADDR = 284;
constexpr int HTTP_SERVER_EEPROM_ADDR = 380;
constexpr int DEVICE_ID_ADDR = 444;
constexpr int DEVICE_LIST_ADDR = 448;

// ==== Root CA для api.telegram.org ====
const char TELEGRAM_CERT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIQRmU2ZBIykkBHzSm6mRypxzANBgkqhkiG9w0BAQsFADA/
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xGDAWBgNVBAMT
D1Jvb3QgQ0EgUm9vdCAtIEcyMB4XDTIwMDYyNTAwMDAwMFoXDTMwMDYyNDIzNTk1
OVowPzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRgwFgYD
VQQDEw9Sb290IENBIFJvb3QgLSBHMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
AQoCggEBAKypx2vwEjh23UNMpddT+2sW0kTLaCkqn5Z8S0S3Vfhz9R+oTBtC1c6C
GZnKr6KPyHOvW+IEdHTrnDUudY1zM04tWKh5+AqqcDKZykB1WUNvbxi5zsxgJK2/
g04QLjHqqE/5bN0msYAXIbFTleYhA7aMne9wuf3ls7n1FUyQrrKJmwhXWf3KTz1A
VyeC1VGKRDcrEX6FxN2dXrPYAz5+0I0z+od8k/RAOU/yU35g8sbQ0BYII40di6KI
vF/jbpDFH9zAyA+3gswHDb6uB2SVuOiG7csV0rOW7jQ+zY7K1o1uNpxvQiluzpXL
f75E+f4Z9M6dFbTD+kQoh9ev9X49wscCAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB
/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYEFNg+JNnx18t1pHABH6Y/2LG2F8qw
MA0GCSqGSIb3DQEBCwUAA4IBAQCZpEaj2lzF+enKnnOgzmIHmBd5dtn8rYJQ2v4f
C3y5KP8v8PZKxDSHLVYRIlnJrped1IovnHgwlHGawEq+y3OC/YLXTr4Wr9L5xRcm
F8UOB7hZPH/LW0OX2uUVksNzx8dx4qPuIgz8fcwTN82ZMKmDG4GJHzFg3Ajc6NNC
pUg/6QMQ6MCTQ2xwbGHptRNZB+Yh8Ejua+ZC3WnSNUFjL0kJ4kgE3AQUrdRr+88j
cZMcG2JSe2xJJHptNwacrkT4VLT2KT+6U6FqVuwZYaUysJ/k6FOLwObAHTkKQ+39
rMxuF8k+nMB0Igf4lhkSNuL6Q8v2sG9FL4+bQMbsQzH/66pF
-----END CERTIFICATE-----
)EOF";
