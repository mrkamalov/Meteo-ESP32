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
        Serial.printf("Failed to fetch config.txt, HTTP code: %d\n", httpCode);
        http.end();
        return false;
    }

    String config = http.getString();
    http.end();
    config.trim();

    int spaceIdx = config.indexOf(' ');
    if (spaceIdx == -1) {
        Serial.println("Invalid config.txt format! No space separator.");
        return false;
    }

    version = config.substring(0, spaceIdx);
    String crcStr = config.substring(spaceIdx + 1);
    crcStr.trim();

    // Проверка формата версии
    if (!isValidVersionFormat(version)) {
        Serial.printf("Invalid version format: %s\n", version.c_str());
        return false;
    }

    crc = strtoul(crcStr.c_str(), nullptr, 10);
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
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("Failed to download file from %s, code: %d\n", url.c_str(), httpCode);
        http.end();
        return false;
    }

    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open local file for writing");
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[128];
    int len;
    while ((len = stream->available())) {
        int r = stream->readBytes(buf, min(len, 128));
        file.write(buf, r);
    }

    file.close();
    http.end();
    return true;
}

bool WiFiUpdater::updateFirmware() {
    Serial.println("Checking for firmware update...");    

    String remoteVersion;
    uint32_t remoteCRC;

    if (!fetchRemoteConfig(remoteVersion, remoteCRC)) {
        Serial.println("Failed to fetch remote config.");
        return false;
    }

    String localVersion = readEEPROMVersion();
    Serial.printf("Local version: %s, Remote version: %s\n", localVersion.c_str(), remoteVersion.c_str());

    if (remoteVersion == "" || remoteVersion == localVersion) {
        Serial.println("No update required.");
        return false;
    }

    if (!initSpiffs()) return false;

    if (!downloadFile(String(HTTP_SERVER) + "/" + FILE_NAME, LOCAL_FILE)) {
        Serial.println("Failed to download firmware.");
        return false;
    }
    
    uint32_t localCRC = calculateLocalCRC32(LOCAL_FILE);

    Serial.printf("Remote CRC: %08X, Local CRC: %08X\n", remoteCRC, localCRC);

    if (remoteCRC == 0 || remoteCRC != localCRC) {
        Serial.println("CRC mismatch, aborting update.");
        return false;
    }

    performFirmwareUpdate(remoteVersion);
    
    return true;
}

void WiFiUpdater::performFirmwareUpdate(String& newVersion) {
    File updateBin = SPIFFS.open(LOCAL_FILE);
    if (!updateBin) {
        Serial.println("Failed to open firmware file.");
        return;
    }

    if (!Update.begin(updateBin.size())) {
        Serial.println("Update.begin failed");
        Update.printError(Serial);
        return;
    }

    size_t written = Update.writeStream(updateBin);
    if (written != updateBin.size()) {
        Serial.println("Firmware write failed!");
        Update.printError(Serial);
        return;
    }

    if (!Update.end()) {
        Serial.println("Update.end failed!");
        Update.printError(Serial);
        return;
    }
    writeEEPROMVersion(newVersion);
    Serial.println("Firmware update successful. Restarting...");
    delay(1000);
    ESP.restart();
}

