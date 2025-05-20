#include "WiFiUpdater.h"
#include <EEPROM.h>
#include <SPIFFS.h>
#include "sim868Config.h"
#include "ServerSettings.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "CRC32Utils.h"
#include <Update.h>
#include <FS.h>

WiFiUpdater::WiFiUpdater() {}

bool WiFiUpdater::initSpiffs() {    
    if (!SPIFFS.begin(true)) {
        SerialMon.println("SPIFFS Mount Failed");
        return false;
    }
    if (SPIFFS.format()) {
        SerialMon.println("SPIFFS formatted successfully!");
        return true;
    } else {
        SerialMon.println("SPIFFS format failed!");
        return false;
    }
}

String WiFiUpdater::readEEPROMVersion() {
    char buffer[10];
    for (int i = 0; i < FW_VERSION_SIZE; i++) {
        buffer[i] = EEPROM.read(FW_VERSION_ADDR + i);
        if (buffer[i] == '\0'|| buffer[i] == 0xFF) break;
    }
    return String(buffer);
}

void WiFiUpdater::writeEEPROMVersion(const String& version) {
    for (int i = 0; i < version.length() && i < 9; i++) {
        EEPROM.write(FW_VERSION_ADDR + i, version[i]);
    }
    EEPROM.write(FW_VERSION_ADDR + version.length(), '\0');
    EEPROM.commit();
}

bool WiFiUpdater::fetchRemoteConfig(String& version, uint32_t& crc) {
    HTTPClient http;
    http.begin(String(HTTP_SERVER) + "/" + CONFIG_FILE); // config.txt
    int httpCode = http.GET();
    if (httpCode != 200) {
        SerialMon.printf("Failed to fetch config.txt, HTTP code: %d\n", httpCode);
        http.end();
        return false;
    }

    String config = http.getString();
    http.end();
    config.trim();
    SerialMon.printf("Fetched config: %s\n", config.c_str());

    int spaceIdx = config.indexOf(' ');
    if (spaceIdx == -1) {
        SerialMon.println("Invalid config.txt format! No space separator.");
        return false;
    }

    version = config.substring(0, spaceIdx);
    String crcStr = config.substring(spaceIdx + 1);
    crcStr.trim();

    // Проверка формата версии
    if (!isValidVersionFormat(version)) {
        SerialMon.printf("Invalid version format: %s\n", version.c_str());
        return false;
    }

    crc = (uint32_t)strtoul(crcStr.c_str(), nullptr, 16);
    return true;
}

bool WiFiUpdater::isValidVersionFormat(const String& version) {
    int dot1 = version.indexOf('.');
    int dot2 = version.indexOf('.', dot1 + 1);

    // Должно быть две точки
    if (dot1 == -1 || dot2 == -1 || version.indexOf('.', dot2 + 1) != -1) {
        return false;
    }

    String major = version.substring(0, dot1);
    String minor = version.substring(dot1 + 1, dot2);
    String patch = version.substring(dot2 + 1);

    // Все три части должны быть числами
    return major.length() > 0 && minor.length() > 0 && patch.length() > 0 &&
           major.toInt() >= 0 && minor.toInt() >= 0 && patch.toInt() >= 0;
}


uint32_t WiFiUpdater::calculateLocalCRC32(const char* path) {
    initCRCTable();
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) return 0;

    uint32_t crc = 0xFFFFFFFF;
    while (file.available()) {
        crc = updateCRC32(crc, file.read());
    }
    file.close();
    return ~crc;
}

bool WiFiUpdater::downloadFile(const String& url, const char* path) {
    int totalLength;       //total size of firmware

    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        SerialMon.println("Failed to open local file for writing");        
        return false;
    }

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != 200) {
        SerialMon.printf("Failed to download file from %s, code: %d\n", url.c_str(), httpCode);
        http.end();
        return false;
    }
    totalLength = http.getSize();
    int len = totalLength;
    int bytesRead = 0; // bytes read so far
    SerialMon.printf("File size: %d bytes\n", totalLength);
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[128];
    while(http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();
        if(size) {
            // read up to 128 byte
            bytesRead += (size > sizeof(buf))? sizeof(buf) : size;
            int c = stream->readBytes(buf, ((size > sizeof(buf)) ? sizeof(buf) : size));
            // pass to function
            file.write(buf, c);
            if(len > 0) {
                len -= c;
            }
        }
        delay(1);
    }    
    SerialMon.printf("Downloaded %d bytes\n", bytesRead);
    file.close();
    http.end();
    return true;
}

bool WiFiUpdater::updateFirmware() {
    SerialMon.println("Checking for firmware update...");    

    String remoteVersion;
    uint32_t remoteCRC;

    if (!fetchRemoteConfig(remoteVersion, remoteCRC)) {
        SerialMon.println("Failed to fetch remote config.");
        return false;
    }

    String localVersion = readEEPROMVersion();
    SerialMon.printf("Local version: %s, Remote version: %s\n", localVersion.c_str(), remoteVersion.c_str());

    if (remoteVersion == "" || remoteVersion == localVersion) {
        SerialMon.println("No update required.");
        return false;
    }

    if (!initSpiffs()) return false;

    if (!downloadFile(String(HTTP_SERVER) + "/" + FILE_NAME, LOCAL_FILE)) {
        SerialMon.println("Failed to download firmware.");
        return false;
    }
    
    uint32_t localCRC = calculateLocalCRC32(LOCAL_FILE);

    SerialMon.printf("Remote CRC: %08X, Local CRC: %08X\n", remoteCRC, localCRC);

    if (remoteCRC == 0 || remoteCRC != localCRC) {
        SerialMon.println("CRC mismatch, aborting update.");
        return false;
    }

    performFirmwareUpdate(remoteVersion);
    
    return true;
}

void WiFiUpdater::performFirmwareUpdate(String& newVersion) {
    File updateBin = SPIFFS.open(LOCAL_FILE);
    if (!updateBin) {
        SerialMon.println("Failed to open firmware file.");
        return;
    }

    if (!Update.begin(updateBin.size())) {
        SerialMon.println("Update.begin failed");
        Update.printError(SerialMon);
        return;
    }

    size_t written = Update.writeStream(updateBin);
    if (written != updateBin.size()) {
        SerialMon.println("Firmware write failed!");
        Update.printError(SerialMon);
        return;
    }

    if (!Update.end()) {
        SerialMon.println("Update.end failed!");
        Update.printError(SerialMon);
        return;
    }
    writeEEPROMVersion(newVersion);
    SerialMon.println("Firmware update successful. Restarting...");
    delay(1000);
    ESP.restart();
}

