#include "Sim800Updater.h"
#include <ArduinoHttpClient.h>
#include <Update.h>
#include "const.h"
#include <FS.h>
#include <SPIFFS.h>

Sim800Updater::Sim800Updater(){}

void Sim800Updater::performFirmwareUpdate() {
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
        ESP.restart();
    }

    firmware.close();
}

void Sim800Updater::initCRCTable() {
    const uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crcTable[i] = crc;
    }
}

uint32_t Sim800Updater::updateCRC32(uint32_t crc, uint8_t data) {
    return (crc >> 8) ^ crcTable[(crc ^ data) & 0xFF];
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

uint32_t Sim800Updater::fetchCRC32() {    
    sendCommand("AT+HTTPINIT");  // Инициализация HTTP
    sendCommand("AT+HTTPPARA=\"CID\",1");  // Выбор профиля GPRS
    sendCommand("AT+HTTPPARA=\"URL\",\"" + String(configUrl) + "\"");
    sendCommand("AT+HTTPSSL=1");  // Включить SSL
    sendCommand("AT+HTTPACTION=0");  // Отправка HTTP GET
    if (!waitForResponse("+HTTPACTION: 0,200", 10000)) {
        Serial.println("HTTP GET Failed");
        sendCommand("AT+HTTPTERM");  // Завершение HTTP-сессии
        return 0;
    }
    Serial1.println("AT+HTTPREAD");
    delay(1000);
    String response = "";
    while (Serial1.available()) {
        char c = Serial1.read();
        response += c;
    }    
    uint32_t crc32_value = 0;
    int responseIndex = response.indexOf("+HTTPREAD: ");
    int startIndex = response.indexOf("\r\n", responseIndex);
    if (startIndex != -1) {
        startIndex += 2;  // Пропускаем "\r\n"
        int endIndex = response.indexOf("\r\n", startIndex);  // Ищем конец числа
        if (endIndex != -1) {
            String crcString = response.substring(startIndex, endIndex);
            sscanf(crcString.c_str(), "%x", &crc32_value);
        }
    }
    Serial.printf("CRC32: 0x%08X\n", crc32_value);
    sendCommand("AT+HTTPTERM");  // Завершение HTTP-сессии
    return crc32_value;
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
    sendCommand("AT+FTPCLOSE", 5000);  // Закрытие FTP-сессии
    SerialMon.println("Download complete!");
    SerialMon.print("Total bytes downloaded: ");
    SerialMon.println(totalBytes);
    return true;
}

bool Sim800Updater::updateFirmwareViaGPRS() {
    initCRCTable();
    if (!initSpiffs()) return false;

    if (!downloadFirmware()) {
        SerialMon.println("Ошибка загрузки файла прошивки!");
        return false;
    }

    uint32_t serverCRC = fetchCRC32();
    uint32_t localCRC = calculateFileCRC32(LOCAL_FILE);

    SerialMon.printf("CRC32 локального файла: 0x%08X\n", localCRC);
    if (serverCRC == 0 || serverCRC != localCRC) {
        SerialMon.println("Ошибка: CRC32 не совпадает!");
        return false;
    }

    SerialMon.println("CRC32 совпадает!");
    performFirmwareUpdate();
    return true;
}

bool Sim800Updater::checkForUpdates(){
    return false;
} 