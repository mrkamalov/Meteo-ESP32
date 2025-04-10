#ifndef METEO_CONFIG_PORTAL_H
#define METEO_CONFIG_PORTAL_H

#include <vector>
#include <ElegantOTA.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

struct MeteoDevice {
        int id;
        String name;
    };

class MeteoConfigPortal {
public:
    MeteoConfigPortal();
    void begin(); // Запуск портала
    void loop();
private:
    // EEPROM config
    static const int EEPROM_SIZE = 512;
    static const int DEVICE_ID_ADDR = 50;
    static const int DEVICE_LIST_ADDR = 54;
    static const int WIFI_SSID_ADDR = 10;
    static const int WIFI_PASS_ADDR = 30;

    AsyncWebServer server;
    std::vector<struct MeteoDevice> meteoDevices;
    int deviceId; 
    String wifiSSID;
    String wifiPass;

    void setupWebServer();
    String getMeteoDevicesList();
    void saveDevicesToEEPROM();
    void loadDevicesFromEEPROM();
    void saveDeviceId(int id);
    String getDeviceId();

    void loadWiFiSettings();
    void saveWiFiSettings(const String& ssid, const String& pass);
    bool connectToWiFi();

    void handleSetDeviceId(AsyncWebServerRequest *request);
    void handleGetDevices(AsyncWebServerRequest *request);
    void handleAddDevice(AsyncWebServerRequest *request);
    void handleRemoveDevice(AsyncWebServerRequest *request);    
    void handleWiFiSettings(AsyncWebServerRequest *request);
};

#endif
