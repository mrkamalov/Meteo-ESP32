#include "FirmwareUpdater.h"
#include <SPIFFS.h>
#include <Update.h>
#include <CRC32.h>
#include "Const.h"

// Initialize SPIFFS
bool initSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return false;
    }
    Serial.println("SPIFFS successfully initialized!");
    return true;
}

// Save the firmware string to SPIFFS
bool saveFirmwareToSPIFFS(const String& data, const char* firmware_file_path) {
    File firmwareFile = SPIFFS.open(firmware_file_path, FILE_WRITE);
    if (!firmwareFile) {
        Serial.println("Error opening file for writing.");
        return false;
    }

    size_t bytesWritten = firmwareFile.print(data);
    firmwareFile.close();

    if (bytesWritten == data.length()) {
        Serial.printf("Successfully written %d bytes.\n", bytesWritten);
        return true;
    } else {
        Serial.printf("Write error: written %d bytes out of %d.\n", bytesWritten, data.length());
        return false;
    }
}

// Verify the checksum of the firmware file
bool verifyFirmwareChecksum(const char* filePath, uint32_t expected_crc32) {
    File firmwareFile = SPIFFS.open(filePath, FILE_READ);
    if (!firmwareFile) {
        Serial.println("Error opening file for checksum verification.");
        return false;
    }

    CRC32 crc;
    uint8_t buffer[128];
    while (firmwareFile.available()) {
        size_t bytesRead = firmwareFile.read(buffer, sizeof(buffer));
        crc.update(buffer, bytesRead);
    }

    firmwareFile.close();
    uint32_t actualCRC = crc.finalize();
    Serial.printf("File checksum: 0x%08X\n", actualCRC);

    return actualCRC == expected_crc32;
}

// Write the firmware from SPIFFS to the OTA partition
bool writeFirmwareToOTA(const char* filePath) {
    File firmwareFile = SPIFFS.open(filePath, FILE_READ);
    if (!firmwareFile) {
        Serial.println("Error opening file for OTA update.");
        return false;
    }

    size_t firmwareSize = firmwareFile.size();
    Serial.printf("Firmware size: %d bytes\n", firmwareSize);

    if (!Update.begin(firmwareSize)) {
        Serial.println("Error: insufficient memory for update.");
        firmwareFile.close();
        return false;
    }

    uint8_t buffer[128];
    size_t totalBytes = 0;

    while (firmwareFile.available()) {
        size_t bytesRead = firmwareFile.read(buffer, sizeof(buffer));
        if (Update.write(buffer, bytesRead) != bytesRead) {
            Serial.println("Error writing data to OTA.");
            Update.abort();
            firmwareFile.close();
            return false;
        }
        totalBytes += bytesRead;
    }

    firmwareFile.close();

    if (Update.end()) {
        if (Update.isFinished()) {
            Serial.println("OTA update successfully completed!");
            return true;
        } else {
            Serial.println("Error: update not completed.");
        }
    } else {
        Serial.printf("Update completion error: %s\n", Update.errorString());
    }

    return false;
}
#error "Split firmware not tested"
// Perform full firmware update (save, verify, write to OTA)
void performFirmwareUpdate(const String& firmwareData, const char* firmware_file_path, uint32_t expected_crc32) {
    Serial.println("Starting firmware update process...");

    // Save firmware to SPIFFS
    if (saveFirmwareToSPIFFS(firmwareData, firmware_file_path)) {
        Serial.println("Firmware saved to SPIFFS.");

        // Verify checksum
        if (verifyFirmwareChecksum(firmware_file_path, expected_crc32)) {
            Serial.println("Firmware checksum is valid.");

            // Write firmware to OTA and update
            if (writeFirmwareToOTA(firmware_file_path)) {
                Serial.println("Firmware updated successfully. Restarting...");
                ESP.restart();
            } else {
                Serial.println("Firmware update failed.");
            }
        } else {
            Serial.println("Error: checksum mismatch.");
        }
    } else {
        Serial.println("Error saving firmware to SPIFFS.");
    }
}
