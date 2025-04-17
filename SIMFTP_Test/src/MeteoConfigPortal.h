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
    void loop();
private:
    // EEPROM config
    static const int EEPROM_SIZE = 512;    
    static const int WIFI_SSID_ADDR = 10;
    static const int WIFI_PASS_ADDR = 30;
    static const int GPRS_APN_ADDR = 50;
    static const int GPRS_USER_ADDR = 70;
    static const int GPRS_PASS_ADDR = 90;
    static const int TRANSFER_PRIORITY_ADDR  = 110;
    static const int DEVICE_ID_ADDR = 114;
    static const int DEVICE_LIST_ADDR = 118;

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
    void saveTransferPriority(int priority);
    void loadTransferPriority();

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
};;

#endif
