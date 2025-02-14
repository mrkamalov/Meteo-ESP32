#include "FirmwareUpdater.h"
#include <SPIFFS.h>
#include <Update.h>
#include <CRC32.h>
#include "Const.h"
#include "GSM.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

String firmware_catalog_path = "/firmware";
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
/*bool saveFirmwareToSPIFFS(const String& data, uint32_t partNum) {
    File firmwareFile;
    if (partNum == 0) {
        firmwareFile = SPIFFS.open(firmware_file_path, FILE_WRITE);
    } else {
        firmwareFile = SPIFFS.open(firmware_file_path, FILE_APPEND);
    }

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
}*/

// Verify the checksum of the firmware file
bool verifyFirmwareChecksum(uint32_t expected_crc32, uint32_t totalSize, const String& firmware_file_path) {
    File firmwareFile = SPIFFS.open(firmware_file_path, FILE_READ);
    if (!firmwareFile) {
        Serial.println("Error opening file for checksum verification.");
        return false;
    }

    size_t firmwareSize = firmwareFile.size();
    if (firmwareSize != totalSize) {
        Serial.printf("Error: firmware size mismatch. Expected: %u, Actual: %u\n", totalSize, firmwareSize);
        firmwareFile.close();
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
bool writeFirmwareToOTA(const String& firmware_file_path) {
    File firmwareFile = SPIFFS.open(firmware_file_path, FILE_READ);
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

// Check if the firmware file size matches the expected part size
bool checkFirmwarePartSize(File& firmwareFile, uint32_t expectedSize) {
    size_t actualSize = firmwareFile.size();
    if (actualSize != expectedSize) {
        Serial.printf("Error: firmware part size mismatch. Expected: %u, Actual: %u\n", expectedSize, actualSize);
        return false;
    }
    return true;
}

void performFirmwareUpdate(void){
    uint32_t partsCount = 0;
    uint32_t crc = 0;
    uint32_t totalSize = 0;
    uint32_t partsSize = 0;

    getConfigData(partsCount, crc, totalSize, partsSize);
    Serial.printf("Total parts: %d, CRC: 0x%08X, Total size: %u bytes Parts size: %u\n", 
                partsCount, crc, totalSize, partsSize);    
    for (int partNum = 0; partNum < partsCount; partNum++) {
        char partNumberString[4];
        snprintf(partNumberString, sizeof(partNumberString), "%03d", partNum);
        String firmwarePath = firmware_catalog_path+String(partNumberString)+".bin";
        File firmwareFile = SPIFFS.open(firmwarePath, FILE_WRITE);
        if (!firmwareFile) {  
            Serial.println("Error opening firmware file.");
            return;
        }
        if(loadFirmwarePart(partNum, firmwareFile)) Serial.println("Firmware part loaded to SPIFFS.");
        else {
            Serial.println("Error loading firmware part.");
            firmwareFile.close();
            return;
        }
        if(!checkFirmwarePartSize(firmwareFile, partsSize)) partNum--;         
        firmwareFile.close();      
    }    
    // Verify checksum
    if (verifyFirmwareChecksum(crc, totalSize, firmware_catalog_path+"/firmware.bin")) {
        Serial.println("Firmware checksum is valid.");

        // Write firmware to OTA and update
        if (writeFirmwareToOTA(firmware_catalog_path+"/firmware.bin")) {
            Serial.println("Firmware updated successfully. Restarting...");
            ESP.restart();
        } else {
            Serial.println("Firmware update failed.");
        }
    } else {
        Serial.println("Error: checksum mismatch.");
    }
}