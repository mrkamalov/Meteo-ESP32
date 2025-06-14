#pragma once

#include <Arduino.h>

class WiFiUpdater {
public:
    WiFiUpdater();    
    bool updateFirmware();    
    void begin();
private:
    bool initSpiffs();
    String readEEPROMVersion();
    void writeEEPROMVersion(const String& version);
    bool downloadFile(const String& url, const char* path);
    bool fetchRemoteConfig(String& version, uint32_t& crc);
    bool isValidVersionFormat(const String& version);
    uint32_t calculateLocalCRC32(const char* path);   
    
    void performFirmwareUpdate(String& newVersion);  // заглушка
    char HTTP_SERVER[64] = {0}; // URL сервера обновлений
};

