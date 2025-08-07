#include "Sim800Updater.h"
#include <Update.h>
#include "const.h"
#include <FS.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include "CRC32Utils.h"
#include "ServerSettings.h"
#include "SerialMon.h"
#include <vector>

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

bool Sim800Updater::downloadFileFromFtp(const String& remoteFilename, int partSize, bool isLastPart) {// TODO: Test FTP download
    File file = SPIFFS.open("/"+remoteFilename, FILE_WRITE);    
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
    sendCommand("AT+FTPGETNAME=\"" + remoteFilename + "\"");
    sendCommand("AT+FTPGETPATH=\"/\"");
    sendCommand("AT+FTPGET=1");  // Начинаем загрузку файла
    SerialMon.printf("Starting download %s from FTP server\n", remoteFilename.c_str());
    isLastPart? SerialMon.println("Last part of the file") : SerialMon.printf("Part size: %d bytes\n", partSize);
    // Ожидание подтверждения от модема
    delay(5000);
    waitForResponse("+FTPGET: 1,1\r\n", 15000);
    int totalBytes = 0;    
    while (true) {        
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
                    // SerialMon.printf("%c", c);
                    totalBytes++;
                }                
                SerialMon.printf("Total bytes written: %d\n", totalBytes);
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

// bool Sim800Updater::updateFirmwareViaGPRS() {
//     String newVersion = "";
//     uint32_t serverCRC;

//     if(checkForUpdates(newVersion, serverCRC)) {
//         SerialMon.println("Обновление доступно!");
//     } else {        
//         return false;
//     }
//     initCRCTable();
//     if (!initSpiffs()) return false;

//     if (!downloadFirmware()) {
//         SerialMon.println("Ошибка загрузки файла прошивки!");
//         return false;
//     }
    
//     uint32_t localCRC = calculateFileCRC32(LOCAL_FILE);

//     SerialMon.printf("CRC32 локального файла: 0x%08X\n", localCRC);
//     if (serverCRC == 0 || serverCRC != localCRC) {
//         SerialMon.println("Ошибка: CRC32 не совпадает!");
//         return false;
//     }

//     SerialMon.println("CRC32 совпадает!");
//     performFirmwareUpdate(newVersion);
//     return true;
// }

bool Sim800Updater::mergeFirmwareParts(int totalParts, const char *outputFile) {
    File outFile = SPIFFS.open(outputFile, FILE_WRITE);
    if (!outFile) {
        SerialMon.printf("Ошибка: не удалось создать %s\n", outputFile);
        return false;
    }

    const size_t bufSize = 1024;
    uint8_t buf[bufSize];

    for (int i = 0; i < totalParts; i++) {
        char partName[32];
        sprintf(partName, "/firmware_part%03d.bin", i);

        if (!SPIFFS.exists(partName)) {
            SerialMon.printf("Часть %s не найдена\n", partName);
            outFile.close();
            return false;
        }

        File partFile = SPIFFS.open(partName, FILE_READ);
        if (!partFile) {
            SerialMon.printf("Не удалось открыть часть %s\n", partName);
            outFile.close();
            return false;
        }

        SerialMon.printf("Объединение части %d: %s\n", i, partName);

        // копирование содержимого в firmware.bin
        while (partFile.available()) {
            size_t bytesRead = partFile.read(buf, bufSize);
            outFile.write(buf, bytesRead);
        }

        partFile.close();

        // удаление части после записи
        if (!SPIFFS.remove(partName)) {
            SerialMon.printf("Не удалось удалить часть %s\n", partName);
        } else {
            SerialMon.printf("Удалена часть %s\n", partName);
        }
    }

    outFile.close();
    SerialMon.printf("Файл прошивки %s успешно создан\n", outputFile);
    return true;
}

void Sim800Updater::updateFirmwareViaGPRS() {
    switch (updateState) {
        case IDLE:
            SerialMon.println("[IDLE] Запуск обновления...");
            fwVersion = "";
            fwCRC = 0;
            fwParts = 0;
            fwPartSize = 0;
            currentPart = 0;
            downloadAttempts = 0;          
            updateState = LOAD_CONFIG_INITIAL;
            break;

        case LOAD_CONFIG_INITIAL: {
            String ver; uint32_t crc; int parts, size;
            if (checkForUpdates(ver, crc, parts, size)) {
                fwVersion = ver;
                fwCRC = crc;
                fwParts = parts;
                fwPartSize = size;
                SerialMon.printf("Обновление найдено: версия %s, CRC: 0x%08X, частей: %d, размер части: %d\n",
                                 fwVersion.c_str(), fwCRC, fwParts, fwPartSize);
               initCRCTable();
               if (!initSpiffs()) updateState = IDLE;
               else updateState = DOWNLOAD_PART;                
            } else {
                SerialMon.println("Обновление не найдено. Ожидание следующей проверки...");                
            }
            break;
        }

        case DOWNLOAD_PART: {
            // Повторно проверяем, не изменилась ли конфигурация
            String ver; uint32_t crc; int parts, size;
            if (!checkForUpdates(ver, crc, parts, size)) {
                SerialMon.println("Ошибка загрузки конфигурации.");
                updateState = IDLE;
                break;
            }

            if (ver != fwVersion || crc != fwCRC || parts != fwParts || size != fwPartSize) {
                SerialMon.println("Конфигурация изменилась. Перезапуск...");
                updateState = IDLE;
                break;
            }            

            char partName[32];
            sprintf(partName, "firmware_part%03d.bin", currentPart);
            SerialMon.printf("Загрузка части %d: %s\n", currentPart, partName);

            if (!downloadFileFromFtp(partName, fwPartSize, (currentPart == fwParts - 1))) {
                SerialMon.printf("Ошибка загрузки части %d\n", currentPart);
                updateState = IDLE;
                break;
            }
            uint32_t fileCRC = calculateFileCRC32(("/" + String(partName)).c_str());
            SerialMon.printf("CRC части %d: 0x%08X\n", currentPart, fileCRC);
            if(fileCRC != partCRCs[currentPart]) {
                SerialMon.printf("Ошибка: CRC части %d не совпадает! Ожидалось: 0x%08X, получено: 0x%08X\n",
                                 currentPart, partCRCs[currentPart], fileCRC);
                downloadAttempts++;
                if(downloadAttempts>5) updateState = IDLE;
                break;
            }
            else currentPart++;            
            if (currentPart >= fwParts) {
                if (mergeFirmwareParts(fwParts, "/firmware.bin")) {
                    SerialMon.println("Все части объединены в firmware.bin");
                    updateState = VERIFY_CRC;
                } else {
                    SerialMon.println("Ошибка при объединении частей");
                    updateState = IDLE;
                }                
            }
            break;
        }

        case VERIFY_CRC: {
            String ver; uint32_t crc; int parts, size;
            if (!checkForUpdates(ver, crc, parts, size)) {
                SerialMon.println("Ошибка загрузки конфигурации перед CRC.");
                updateState = IDLE;
                break;
            }

            if (ver != fwVersion || crc != fwCRC || parts != fwParts || size != fwPartSize) {
                SerialMon.println("Конфигурация изменилась перед CRC. Перезапуск...");
                updateState = IDLE;
                break;
            }

            uint32_t localCRC = calculateFileCRC32(LOCAL_FILE);
            SerialMon.printf("CRC локального файла: 0x%08X\n", localCRC);

            if (fwCRC == 0 || fwCRC != localCRC) {
                SerialMon.println("Ошибка: CRC не совпадает.");
                updateState = IDLE;
                break;
            }

            SerialMon.println("CRC совпадает. Обновляем прошивку...");
            performFirmwareUpdate(fwVersion);
            updateState = UPDATE_DONE;
            break;
        }

        case UPDATE_DONE:
            SerialMon.println("Обновление завершено успешно.");
            updateState = IDLE;
            break;        
    }
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

bool Sim800Updater::fetchConfigFromFTP(String &version, uint32_t &remoteCRC, int &partsCount, int &partSize) {
    SerialMon.println("Загрузка config.txt из FTP...");

    // Настройка FTP
    sendCommand("AT+FTPCID=1");
    sendCommand("AT+FTPSERV=\"" + String(FTP_SERVER) + "\"");
    sendCommand("AT+FTPUN=\"" + String(FTP_USER) + "\"");
    sendCommand("AT+FTPPW=\"" + String(FTP_PASS) + "\"");
    sendCommand("AT+FTPTYPE=A");  // текстовый режим
    sendCommand("AT+FTPGETNAME=\"config.txt\"");
    sendCommand("AT+FTPGETPATH=\"/\"");

    // Запрос файла
    sendCommand("AT+FTPGET=1");
    delay(5000);
    if (!waitForResponse("+FTPGET: 1,1", 10000)) {
        SerialMon.println("Ошибка получения config.txt");
        return false;
    }

    String response;
    // Чтение блоками
    while (true) {
        SerialAT.println("AT+FTPGET=2,512");
        delay(500);
        String chunk;
        while (SerialAT.available()) {
            chunk += (char)SerialAT.read();
        }
        if (chunk.length() == 0) break; // данные закончились
        response += chunk;
        if (chunk.indexOf("+FTPGET: 2,0") != -1) break; // конец файла
    }

    SerialMon.println("Получен config.txt:");
    SerialMon.println(response);
    SerialMon.println("Обработка первой строки...");

    // Извлекаем первую строку с полезными данными (пропускаем служебные строки)
    int lineStart = 0;
    String firstLine;

    while (true) {
        int lineEnd = response.indexOf("\r\n", lineStart);
        if (lineEnd == -1) break;

        firstLine = response.substring(lineStart, lineEnd);
        firstLine.trim();

        // Пропустить служебные строки и пустые
        if (firstLine.length() > 0 && !firstLine.startsWith("+FTPGET:") && !firstLine.startsWith("OK")) {
            break;
        }

        lineStart = lineEnd + 2; // Переход на следующую строку
    }

    if (firstLine.length() == 0) {
        SerialMon.println("Не найдена первая строка с данными.");
        return false;
    }

    SerialMon.println("Найдена первая строка:");
    SerialMon.println(firstLine);

    // Парсим первую строку: VERSION CRC PARTS PARTSIZE
    int sep1 = firstLine.indexOf(' ');
    int sep2 = firstLine.indexOf(' ', sep1 + 1);
    int sep3 = firstLine.indexOf(' ', sep2 + 1);
    if (sep1 == -1 || sep2 == -1 || sep3 == -1) {
        SerialMon.println("Неверный формат первой строки.");
        return false;
    }

    version = firstLine.substring(0, sep1);
    String crcStr = firstLine.substring(sep1 + 1, sep2);
    String partsCountStr = firstLine.substring(sep2 + 1, sep3);
    String partSizeStr = firstLine.substring(sep3 + 1);

    if (!isValidVersionFormat(version)) {
        SerialMon.printf("Invalid version format: %s\n", version.c_str());
        return false;
    }

    remoteCRC = (uint32_t)strtoul(crcStr.c_str(), nullptr, 16);
    partsCount = partsCountStr.toInt();
    partSize = partSizeStr.toInt();

    if (remoteCRC == 0 || partsCount <= 0 || partSize <= 0) {
        SerialMon.println("Некорректные значения в первой строке.");
        return false;
    }

    if (partsCount > MAX_PARTS) {
        SerialMon.printf("Ошибка: частей (%d) больше MAX_PARTS (%d)\n", partsCount, MAX_PARTS);
        return false;
    }

    // Разбор CRC частей
    int lineStart = pos;
    int idx = 0;
    while (idx < partsCount) {
        int nextStart = response.indexOf("\r\n", lineStart + 2);
        if (nextStart == -1) break;

        String crcLine = response.substring(lineStart + 2, nextStart);
        crcLine.trim();
        if (crcLine.length() > 0) {
            uint32_t partCrc = (uint32_t)strtoul(crcLine.c_str(), nullptr, 16);
            partCRCs[idx++] = partCrc;
        }
        lineStart = nextStart;
    }

    if (idx != partsCount) {
        SerialMon.printf("Предупреждение: ожидается %d CRC, получено %d\n", partsCount, idx);
        partsCount = idx; // подправим
    }

    // Закрытие FTP
    sendCommand("AT+FTPQUIT", 3000);

    SerialMon.printf("Версия: %s, CRC файла: %08X, Частей: %d, Размер: %d\n",
                     version.c_str(), remoteCRC, partsCount, partSize);
    for (int i = 0; i < partsCount; i++) {
        SerialMon.printf("CRC части %d: %08X\n", i, partCRCs[i]);
    }

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

bool Sim800Updater::checkForUpdates(String &version, uint32_t &remoteCRC, int& partsCount, int& partSize) {
    //return false;
    String currentVersion = readLocalVersion();

    if (!fetchConfigFromFTP(version, remoteCRC, partsCount, partSize)) {
        SerialMon.println("Ошибка загрузки версии и CRC.");
        return false;
    }

    if (version == "" || version == currentVersion || remoteCRC == 0 || partsCount <= 0 || partSize <= 0) {
        SerialMon.println("No update required.");
        return false;
    }

    return true;
}
