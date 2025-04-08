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
    void loop(); // Цикл обработки

private:
    WiFiManager wifiManager;
    AsyncWebServer server;
    std::vector<struct MeteoDevice> meteoDevices;
    int deviceId; 
    bool configPortalRequested = true;
    bool portalRunning = false;
    unsigned int startTime = millis(); // Время начала работы конфигурационного портала
    const int timeout = 100; // Время ожидания в секундах
    bool webPortalActive = false; // Флаг активности веб-портала

    void doWiFiManager();
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
    void handleStartConfigPortal(AsyncWebServerRequest *request);
};

#endif
