#ifndef SIM800_UPDATER_H
#define SIM800_UPDATER_H

#include "deviceConfig.h"
#include <Arduino.h>

class Sim800Updater {
public:
    Sim800Updater();

    void updateFirmwareViaGPRS();
    void begin();   

private:    
    void sendCommand(const String& command, int delayMs = 500);
    bool waitForResponse(const String& expected, int timeout);
    //bool downloadFirmware();
    bool downloadFileFromFtp(const String& remoteFilename, int partSize);
    //bool downloadFileFromFTP(const String& url, const char* path);
    bool fetchConfigFromFTP(String &config);//String &version, uint32_t &remoteCRC, int &partsCount, int &partSize
    String readLocalVersion();
    void saveVersionToEEPROM(const String& version);    
    bool checkForUpdates(String &version, uint32_t &remoteCRC, int& partSize);
    bool isValidVersionFormat(const String& version);    
    bool parseConfigContent(const String& configContent, String& version, uint32_t& totalCrc, int& partSize);
    bool startUpdate(size_t firmwareSize);
    bool writeFirmwareChunk(uint8_t *data, size_t len);
    bool finishUpdate(String& newVersion);

    char FTP_SERVER[32] = {0};
    char FTP_USER[32] = {0};
    char FTP_PASS[32] = {0};
};

#endif
