#include "Sim800Updater.h"
#include <Update.h>
#include "const.h"
#include <FS.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include "CRC32Utils.h"
#include "ServerSettings.h"

Sim800Updater::Sim800Updater(){}

void Sim800Updater::begin() {    
    for (int i = 0; i < 32; ++i) {
        FTP_SERVER[i] = EEPROM.read(FTP_EEPROM_ADDR + i);
        FTP_USER[i] = EEPROM.read(FTP_EEPROM_ADDR + 32 + i);
        FTP_PASS[i] = EEPROM.read(FTP_EEPROM_ADDR + 64 + i);
    }
    FTP_SERVER[31] = '\0';
    FTP_USER[31] = '\0';
    FTP_PASS[31] = '\0';
    
    if (FTP_SERVER[0] == 0xFF || FTP_SERVER[0] == '\0' || FTP_USER == 0 || FTP_PASS[0] == '\0') {
        SerialMon.println("EEPROM is empty, loading default FTP config");

        strncpy(FTP_SERVER, FTP_SERVER_DEFAULT, sizeof(FTP_SERVER));        
        strncpy(FTP_USER, FTP_USER_DEFAULT, sizeof(FTP_USER));
        strncpy(FTP_PASS, FTP_PASS_DEFAULT, sizeof(FTP_PASS));        
    }
    else SerialMon.println("Loaded FTP config from EEPROM");
    SerialMon.printf("FTP Server: %s, User: %s, Pass: %s\n", FTP_SERVER, FTP_USER, FTP_PASS);
}

void Sim800Updater::performFirmwareUpdate(String& newVersion) {
    SerialMon.println("Начало обновления прошивки...");
    File firmware = SPIFFS.open(LOCAL_FILE, FILE_READ);
    if (!firmware) {
        SerialMon.println("Ошибка: не удалось открыть файл прошивки");
        return;
    }

    size_t firmwareSize = firmware.size();
    SerialMon.printf("Размер прошивки: %d байт\n", firmwareSize);

    if (firmwareSize == 0 || !Update.begin(firmwareSize)) {
        SerialMon.println("Ошибка подготовки обновления");
        firmware.close();
        return;
    }

    size_t written = Update.writeStream(firmware);
    if (written != firmwareSize) {
        SerialMon.printf("Ошибка записи: %d из %d байт\n", written, firmwareSize);
        Update.end();
        firmware.close();
        return;
    }

    if (!Update.end() || !Update.isFinished()) {
        SerialMon.println("Ошибка завершения обновления");
    } else {
        SerialMon.println("Обновление завершено. Перезагрузка...");
        delay(1000);
        // Сохраняем версию прошивки в EEPROM        
        saveVersionToEEPROM(newVersion);            
        ESP.restart();
    }

    firmware.close();
}

uint32_t Sim800Updater::calculateFileCRC32(const char* filePath) {
    File file = SPIFFS.open(filePath, FILE_READ);
    if (!file) {
        SerialMon.println("Ошибка открытия файла!");
        return 0;
    }

    uint32_t crc = 0xFFFFFFFF;
    uint8_t buffer[512];
    size_t bytesRead;

    while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            crc = updateCRC32(crc, buffer[i]);
        }
    }

    file.close();
    return crc ^ 0xFFFFFFFF;
}

void Sim800Updater::sendCommand(const String& command, int delayMs) {
    SerialMon.println("AT Command: " + command);
    SerialAT.println(command);
    delay(delayMs);
    while (SerialAT.available()) {
        SerialMon.write(SerialAT.read());
    }
}

bool Sim800Updater::waitForResponse(const String& expectedResponse, int timeout) {
    unsigned long startTime = millis();
    String response = "";
    while (millis() - startTime < timeout) {
        while (SerialAT.available()) {
            char c = SerialAT.read();
            response += c;
            if (response.indexOf(expectedResponse) != -1) {
                SerialMon.println("Response: " + response);
                return true;
            }
        }
    }
    return false;
}

bool Sim800Updater::initSpiffs() {
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

bool Sim800Updater::downloadFirmware (){
    File file = SPIFFS.open(LOCAL_FILE, FILE_WRITE);
    if (!file) {
        SerialMon.println("Failed to open file for writing");
        return false;
    }

    SerialMon.println("Initializing SIM800 FTP...");
    sendCommand("AT+FTPCID=1");
    sendCommand("AT+FTPSERV=\"" + String(FTP_SERVER) + "\"");
    sendCommand("AT+FTPUN=\"" + String(FTP_USER) + "\"");
    sendCommand("AT+FTPPW=\"" + String(FTP_PASS) + "\"");
    sendCommand("AT+FTPTYPE=I");
    sendCommand("AT+FTPGETNAME=\"" + String(FILE_NAME) + "\"");
    sendCommand("AT+FTPGETPATH=\"/\"");
    sendCommand("AT+FTPGET=1");  // Начинаем загрузку файла
    // Ожидание подтверждения от модема
    delay(5000);
    waitForResponse("+FTPGET: 1,1\r\n", 15000);
    int totalBytes = 0;    
    while (true) {
        //SerialMon.println("AT Command: AT+FTPGET=2,256");// Запрос данных
        SerialAT.println("AT+FTPGET=2,256");
        delay(150);//50 ms minimum delay
        String response = "";
        while (SerialAT.available()) {
            char c = SerialAT.read();
            response += c;
        }
        //SerialMon.println(response);
        if (response.indexOf("\r\n+FTPGET: 1,0") != -1) {
            break; // Передача завершена
        }
        
        int startIndex = response.indexOf("+FTPGET: 2,", 2);
        //SerialMon.printf("\nstartIndex: %d\n", startIndex);        
        if (startIndex != -1) {
            startIndex += 11; // Пропускаем "+FTPGET: 2,"            
            int endIndex = response.indexOf("\r\n", startIndex);
            int availableBytes = response.substring(startIndex, endIndex).toInt();
            //SerialMon.printf("availableBytes: %d\n", availableBytes);
            startIndex = endIndex + 2; // Пропускаем "\r\n"
            if (availableBytes > 0) {
                char c;                
                for (int i = startIndex; i < response.length(); i++) {
                    if (i + 1 < response.length() && strncmp(&response[i], "\r\nOK", 4) == 0) {
                        break;
                    }
                    c= response.charAt(i);
                    file.write(response[i]);                    
                    SerialMon.printf("%c", c);
                    totalBytes++;
                }
            }            
        }
    }
    file.close();
    sendCommand("AT+FTPQUIT", 5000);  // Закрытие FTP-сессии
    SerialMon.println("");
    SerialMon.println("");
    SerialMon.println("Download complete!");
    SerialMon.print("Total bytes downloaded: ");
    SerialMon.println(totalBytes);
    return true;
}

bool Sim800Updater::updateFirmwareViaGPRS() {
    String newVersion = "";
    uint32_t serverCRC;

    if(checkForUpdates(newVersion, serverCRC)) {
        SerialMon.println("Обновление доступно!");
    } else {        
        return false;
    }
    initCRCTable();
    if (!initSpiffs()) return false;

    if (!downloadFirmware()) {
        SerialMon.println("Ошибка загрузки файла прошивки!");
        return false;
    }
    
    uint32_t localCRC = calculateFileCRC32(LOCAL_FILE);

    SerialMon.printf("CRC32 локального файла: 0x%08X\n", localCRC);
    if (serverCRC == 0 || serverCRC != localCRC) {
        SerialMon.println("Ошибка: CRC32 не совпадает!");
        return false;
    }

    SerialMon.println("CRC32 совпадает!");
    performFirmwareUpdate(newVersion);
    return true;
}

String Sim800Updater::readLocalVersion() {
    String version = "";
    for (int i = 0; i < FW_VERSION_SIZE; i++) { // максимум 8 символов
        char c = EEPROM.read(FW_VERSION_ADDR + i);
        if (c == '\0' || c == 0xFF) break;
        version += c;
    }
    version.trim();
    SerialMon.printf("Локальная версия: %s\n", version.c_str());
    return version;
}

void Sim800Updater::saveVersionToEEPROM(const String& version) {
    for (int i = 0; i < FW_VERSION_SIZE; i++) {
        if (i < version.length()) {
            EEPROM.write(FW_VERSION_ADDR + i, version[i]);
        } else {
            EEPROM.write(FW_VERSION_ADDR + i, '\0');
        }
    }
    EEPROM.commit();
    SerialMon.println("Версия прошивки сохранена в EEPROM");
}

bool Sim800Updater::fetchConfigFromFTP(String &version, uint32_t &remoteCRC) {
    SerialMon.println("Загрузка версии из FTP...");

    sendCommand("AT+FTPCID=1");
    sendCommand("AT+FTPSERV=\"" + String(FTP_SERVER) + "\"");
    sendCommand("AT+FTPUN=\"" + String(FTP_USER) + "\"");
    sendCommand("AT+FTPPW=\"" + String(FTP_PASS) + "\"");
    sendCommand("AT+FTPTYPE=A");  // Текстовый режим
    sendCommand("AT+FTPGETNAME=\"config.txt\"");
    sendCommand("AT+FTPGETPATH=\"/\"");

    sendCommand("AT+FTPGET=1");
    delay(3000);
    waitForResponse("+FTPGET: 1,1", 10000);

    String response = "";
    delay(300);
    SerialAT.println("AT+FTPGET=2,256");        
    delay(300);
    while (SerialAT.available()) {
        response += (char)SerialAT.read();
    }
    SerialMon.println(response);

    // Парсинг строки
    int dataStart = response.indexOf("+FTPGET: 2,");
    if (dataStart == -1) return false;

    int crlf = response.indexOf("\r\n", dataStart);
    if (crlf == -1) return false;

    int dataBegin = crlf + 2;
    int dataEnd = response.indexOf("\r\n", dataBegin);
    if (dataEnd == -1) return false;

    String line = response.substring(dataBegin, dataEnd);
    line.trim();
    SerialMon.print("Строка конфигурации: ");
    SerialMon.println(line);

    // Ожидаем формат: "1.0.0 1234567890"
    int sep = line.indexOf(' ');
    if (sep == -1) return false;

    version = line.substring(0, sep);
    String crcStr = line.substring(sep + 1);

    // Проверка формата версии
    if (!isValidVersionFormat(version)) {
        Serial.printf("Invalid version format: %s\n", version.c_str());
        return false;
    }
    
    remoteCRC = (uint32_t)strtoul(crcStr.c_str(), nullptr, 16);
    if (remoteCRC == 0) {
        SerialMon.println("Некорректный CRC.");
        return false;
    }

    // Закрытие FTP-сессии
    sendCommand("AT+FTPQUIT", 3000);
    SerialMon.printf("Версия: %s, CRC: %08X\n", version.c_str(), remoteCRC);
    return true;
}

bool Sim800Updater::isValidVersionFormat(const String& version) {
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

bool Sim800Updater::checkForUpdates(String &version, uint32_t &remoteCRC) {
    //return false;
    String currentVersion = readLocalVersion();

    if (!fetchConfigFromFTP(version, remoteCRC)) {
        SerialMon.println("Ошибка загрузки версии и CRC.");
        return false;
    }

    if (version == "" || version == currentVersion) {
        Serial.println("No update required.");
        return false;
    }

    return true;
}
