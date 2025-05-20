#ifndef SIM800_UPDATER_H
#define SIM800_UPDATER_H

#include "sim868Config.h"
#include <Arduino.h>

class Sim800Updater {
public:
    Sim800Updater();

    bool updateFirmwareViaGPRS();    

private:    
    uint32_t calculateFileCRC32(const char* filePath);
    bool initSpiffs();
    void performFirmwareUpdate(String& newVersion);
    void sendCommand(const String& command, int delayMs = 500);
    bool waitForResponse(const String& expected, int timeout);
    bool downloadFirmware();
    bool fetchConfigFromFTP(String &version, uint32_t &remoteCRC);    
    String readLocalVersion();
    void saveVersionToEEPROM(const String& version);    
    bool checkForUpdates(String &version, uint32_t &remoteCRC);
    bool isValidVersionFormat(const String& version);
};

#endif
