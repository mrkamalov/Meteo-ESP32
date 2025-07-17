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

// Enum приоритета передачи данных
enum DataPriority {
    PRIORITY_WIFI_ONLY = 0,
    PRIORITY_GPRS_ONLY = 1,
    PRIORITY_WIFI_THEN_GPRS = 2,
    PRIORITIES_NUMBER = 3
};

class MeteoConfigPortal {
public:
    MeteoConfigPortal();
    void begin(); // Запуск портала
    void loop(const String& sensorJson);
    DataPriority getTransferPriority();
private:  
    AsyncWebServer server;
    std::vector<struct MeteoDevice> meteoDevices;
    int deviceId; 
    String wifiSSID;
    String wifiPass;
    unsigned long lastWifiCheck = 0;
    const unsigned long wifiCheckInterval = 90000;  // 90 секунд
    unsigned long lastApCheck = 0;
    unsigned long apCheckInterval = 60000;
    bool apActive = false;
    bool apRestartPending = false;
    unsigned long apRestartTime = 0;
    const unsigned long apRestartDelay = 100; // 100 ms delay before AP restart
    String gprsAPN = "";
    String gprsUser = "";
    String gprsPass = "";
    DataPriority transferPriority;
    String _cachedSensorJson; 

    void setupWebServer();
    String getMeteoDevicesList();
    void saveDevicesToEEPROM();
    void loadDevicesFromEEPROM();
    void saveDeviceId(int id);
    String getDeviceId();

    void loadWiFiSettings();
    void saveWiFiSettings(const String& ssid, const String& pass);
    bool connectToWiFi();
    void startAccessPoint();
    void saveGPRSSettings(String apn, String user, String pass);
    void loadGPRSSettings();    
    void loadTransferPriority();
    //void saveMQTTToEEPROM(const String& broker, uint16_t port, const String& user, const String& pass, const String& clientId);
    //void loadMQTTFromEEPROM(char* broker, uint16_t& port, char* user, char* pass, char* clientId);
    void saveMQTTToEEPROM(const String& broker, uint16_t port, const String& user, const String& pass, const String& clientId);
    void loadMQTTFromEEPROM(char* broker, uint16_t& port, char* user, char* pass, char* clientId);
    void saveFTPToEEPROM(const String& server, const String& user, const String& pass);
    void saveHttpServerToEEPROM(const String& server);
    void handleRoot();
    void handleSetDeviceId();
    void handleGetDeviceId();
    void handleGetDevices();
    void handleAddDevice();
    void handleRemoveDevice();
    void handleGetWiFi();
    void handleSetWiFi();
    void handleGetGPRS();
    void handleSetGPRS();
    void handleReboot();  
    void handleGetTransferPriority();
    void handleSetTransferPriority();
    void handleGetMQTT();
    void handleSetMQTT();
    void handleGetFTP();
    void handleSetFTP();
    void handleGetHttpServer();
    void handleSetHttpServer();
    void handleGetSensorData();
    void handleSensor();
    void handleSetDefaults();
};

#endif
