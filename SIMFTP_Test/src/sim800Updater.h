#ifndef SIM800_UPDATER_H
#define SIM800_UPDATER_H

#include <Arduino.h>

class Sim800Updater {
public:
    Sim800Updater();
    void init();
    bool checkForUpdates();
    void updateFirmwareViaGPRS();

private:
    void performFirmwareUpdate();    
    void initCRCTable();
    uint32_t updateCRC32(uint32_t crc, uint8_t data);
    uint32_t calculateFileCRC32(const char* filePath);
    void sendCommand(String command, int delayTime = 3000);
    bool waitForResponse(const String& expectedResponse, int timeout = 5000);
    void initSim800();
    bool initSpiffs();
    uint32_t fetchCRC32();
    bool downloadFirmware();

    uint32_t crcTable[256];
};
#endif // SIM800_UPDATER_H
