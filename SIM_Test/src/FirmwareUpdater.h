#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>

bool initSPIFFS();
// bool saveFirmwareToSPIFFS(const String& data);
// bool verifyFirmwareChecksum(const char* filePath, uint32_t expectedCRC);
// bool writeFirmwareToOTA(const char* filePath);
void performFirmwareUpdate(void);
#endif // FIRMWARE_UPDATER_H
