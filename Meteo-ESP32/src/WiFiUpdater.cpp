#include "WiFiUpdater.h"
#include <EEPROM.h>
#include "deviceConfig.h"
#include "ServerSettings.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "SerialMon.h"

WiFiUpdater::WiFiUpdater() {}

void WiFiUpdater::begin() {
    for (int i = 0; i < 64; ++i) {
        HTTP_SERVER[i] = EEPROM.read(HTTP_SERVER_EEPROM_ADDR + i);        
    }
    HTTP_SERVER[63] = '\0';    
    
    if (HTTP_SERVER[0] == 0xFF || HTTP_SERVER[0] == '\0') {
        SerialMon.println("EEPROM is empty, loading default HTTP config");

        strncpy(HTTP_SERVER, HTTP_SERVER_DEFAULT, sizeof(HTTP_SERVER));
    }
    else SerialMon.println("Loaded HTTP config from EEPROM");
    SerialMon.printf("HTTP Server: %s\n", HTTP_SERVER);
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

bool WiFiUpdater::startUpdate(size_t firmwareSize) {
    // Начало обновления, вторая область прошивки
    if (!Update.begin(firmwareSize, U_FLASH)) {
        SerialMon.printf("Update.begin() failed: %s\n", Update.errorString());
        return false;
    }
    SerialMon.println("Update started...");
    return true;
}

bool WiFiUpdater::writeFirmwareChunk(uint8_t *data, size_t len) {
    size_t written = Update.write(data, len);
    if (written != len) {
        SerialMon.printf("Update.write() failed: %s\n", Update.errorString());
        return false;
    }
    return true;
}

bool WiFiUpdater::finishUpdate(String& newVersion) {
    if (!Update.end(true)) { // true = проверка CRC
        SerialMon.printf("Update.end() failed: %s\n", Update.errorString());
        return false;
    }    
    SerialMon.println("Обновление завершено. Перезагрузка...");
    delay(1000);
    // Сохраняем версию прошивки в EEPROM        
    writeEEPROMVersion(newVersion);
    ESP.restart();
    return true;
}

bool WiFiUpdater::downloadFile(const String& url) {
    int totalLength;       //total size of firmware

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
    if(startUpdate(totalLength)) SerialMon.println("Firmware update started successfully.");        
    else return false;
    int bytesRead = 0; // bytes read so far
    SerialMon.printf("File size: %d bytes\n", totalLength);
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[128];
    int lastPercent = -1; // для контроля, чтобы не спамить выводом
    SerialMon.println("Starting download firmware");
    while(http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();
        if(size) {
            // read up to 128 byte
            bytesRead += (size > sizeof(buf))? sizeof(buf) : size;
            int c = stream->readBytes(buf, ((size > sizeof(buf)) ? sizeof(buf) : size));            
            
            if (!writeFirmwareChunk((uint8_t*)&buf, c)) {
                    SerialMon.println("Error writing firmware chunk");
                    return false;
            }
            if(len > 0) {
                len -= c;
            }
            // вычисляем процент
            if (totalLength > 0) {
                int percent = (bytesRead * 100) / totalLength;
                if (percent != lastPercent) {
                    SerialMon.printf("\rProgress: %d%%", percent);
                    lastPercent = percent;
                }
            }
        }
        delay(1);
    }    
    SerialMon.printf("\nDownloaded %d bytes\n", bytesRead);    
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

    if (!downloadFile(String(HTTP_SERVER) + "/" + FILE_NAME)) {
        SerialMon.println("Failed to download firmware.");
        Update.abort();
        return false;
    }    

    if(!finishUpdate(remoteVersion)) return false;
    
    return true;
}

