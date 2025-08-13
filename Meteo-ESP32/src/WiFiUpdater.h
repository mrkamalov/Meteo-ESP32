#pragma once

#include <Arduino.h>

class WiFiUpdater {
public:
    WiFiUpdater();    
    bool updateFirmware();    
    void begin();
private:    
    String readEEPROMVersion();
    void writeEEPROMVersion(const String& version);
    bool downloadFile(const String& url);
    bool fetchRemoteConfig(String& version, uint32_t& crc);
    bool isValidVersionFormat(const String& version);
    bool startUpdate(size_t firmwareSize);
    bool writeFirmwareChunk(uint8_t *data, size_t len);
    bool finishUpdate(String& newVersion);
    
    char HTTP_SERVER[64] = {0}; // URL сервера обновлений
};

