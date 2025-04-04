#ifndef METEO_CONFIG_PORTAL_H
#define METEO_CONFIG_PORTAL_H

#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <vector>

struct MeteoDevice {
        int id;
        String name;
    };

class MeteoConfigPortal {
public:
    MeteoConfigPortal();
    void begin(); // Запуск портала

private:
    WiFiManager wifiManager;
    AsyncWebServer server;
    std::vector<struct MeteoDevice> meteoDevices;
    int deviceId; 

    void setupWebServer();
    String getMeteoDevicesList();
    void saveDevicesToEEPROM();
    void loadDevicesFromEEPROM();
    void saveDeviceId(int id);
    String getDeviceId();
    void handleSetDeviceId(AsyncWebServerRequest *request);
    void handleGetDevices(AsyncWebServerRequest *request);
    void handleAddDevice(AsyncWebServerRequest *request);
    void handleRemoveDevice(AsyncWebServerRequest *request);
};

#endif
