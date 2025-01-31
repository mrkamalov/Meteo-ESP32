#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <Arduino.h>

bool initSPIFFS();
// bool saveFirmwareToSPIFFS(const String& data);
// bool verifyFirmwareChecksum(const char* filePath, uint32_t expectedCRC);
// bool writeFirmwareToOTA(const char* filePath);
void performFirmwareUpdate(const String& firmwareData, const char* firmware_file_path, uint32_t expected_crc32);

#endif // FIRMWARE_UPDATER_H
