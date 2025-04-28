#ifndef SIM800_UPDATER_H
#define SIM800_UPDATER_H

#include "sim868Config.h"
#include <Arduino.h>

class Sim800Updater {
public:
    Sim800Updater();

    bool updateFirmwareViaGPRS();
    bool checkForUpdates();

private:
    void initCRCTable();
    uint32_t updateCRC32(uint32_t crc, uint8_t data);
    uint32_t calculateFileCRC32(const char* filePath);
    bool initSpiffs();
    void performFirmwareUpdate();
    void sendCommand(const String& command, int delayMs = 500);
    bool waitForResponse(const String& expected, int timeout);
    bool downloadFirmware();
    // HTTP CRC fetch
    uint32_t fetchCRC32();

    uint32_t crcTable[256];    
};

#endif
