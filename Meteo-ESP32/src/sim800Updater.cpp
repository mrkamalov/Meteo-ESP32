#include "sim800Updater.h"
#include <HardwareSerial.h>
#include "SPIFFS.h"
#include "Update.h"
#include "const.h"

Sim800Updater::Sim800Updater() {    
}

void Sim800Updater::init(){
    initCRCTable();
    initSim800();
}

void Sim800Updater::performFirmwareUpdate() {
    File firmware = SPIFFS.open(LOCAL_FILE, FILE_READ);
    if (!firmware) {
        Serial.println("Ошибка: не удалось открыть файл прошивки");
        return;
    }

    size_t firmwareSize = firmware.size();
    Serial.printf("Размер прошивки: %d байт\n", firmwareSize);

    if (firmwareSize == 0) {
        Serial.println("Ошибка: файл прошивки пустой!");
        firmware.close();
        return;
    }

    if (!Update.begin(firmwareSize)) {  // Подготовка области OTA
        Serial.println("Ошибка при подготовке обновления");
        firmware.close();
        return;
    }

    Serial.println("Обновление началось...");
    
    size_t written = Update.writeStream(firmware);
    if (written != firmwareSize) {
        Serial.printf("Ошибка записи: записано %d из %d байт\n", written, firmwareSize);
        Update.end();
        firmware.close();
        return;
    }

    if (!Update.end()) {
        Serial.println("Ошибка завершения обновления");
        firmware.close();
        return;
    }

    if (!Update.isFinished()) {
        Serial.println("Обновление не завершено!");
    } else {
        Serial.println("Обновление завершено. Перезагрузка...");
        delay(1000);
        ESP.restart();
    }

    firmware.close();
}

// Инициализация таблицы для быстрого вычисления CRC32
void Sim800Updater::initCRCTable() {
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        crcTable[i] = crc;
    }
}

// Обновление CRC32 на основе нового байта
uint32_t Sim800Updater::updateCRC32(uint32_t crc, uint8_t data) {
    return (crc >> 8) ^ crcTable[(crc ^ data) & 0xFF];
}

// Функция расчета CRC32 для файла в SPIFFS
uint32_t Sim800Updater::calculateFileCRC32(const char* filePath) {
    File file = SPIFFS.open(filePath, FILE_READ);
    if (!file) {
        Serial.println("Ошибка открытия файла!");
        return 0;
    }

    uint32_t crc = 0xFFFFFFFF;  // Начальное значение CRC32
    uint8_t buffer[BUFFER_SIZE];
    size_t bytesRead;

    while ((bytesRead = file.read(buffer, BUFFER_SIZE)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            crc = updateCRC32(crc, buffer[i]);
        }
    }

    file.close();
    return crc ^ 0xFFFFFFFF;  // Инверсия итогового значения
}

void Sim800Updater::sendCommand(String command, int delayTime) {
    Serial.println("AT Command: " + command);
    Serial1.println(command);
    delay(delayTime);
    while (Serial1.available()) {
        Serial.write(Serial1.read());  // Отладочный вывод
    }
}

bool Sim800Updater::waitForResponse(const String& expectedResponse, int timeout) {
    unsigned long startTime = millis();
    String response = "";
    int index;
    while (millis() - startTime < timeout) {
        while (Serial1.available()) {
            char c = Serial1.read();
            Serial.write(c);  // Отладочный вывод
            response += c;
            index = response.indexOf(expectedResponse);
            if (index != -1) {
                Serial.printf("Index %d\n", index);
                return true;
            }
        }
    }
    return false;
}

void Sim800Updater::initSim800(void){
    // Включение питания модема
    pinMode(MODEM_POWER_PIN, OUTPUT);
    digitalWrite(MODEM_POWER_PIN, HIGH);
    Serial.println("SIM800 power on. Wait 6 seconds...");
    delay(6000);
    Serial.println("Initializing SIM800...");
    sendCommand("AT");
    sendCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    sendCommand("AT+SAPBR=3,1,\"APN\",\"" + String(apn) + "\"");
    sendCommand("AT+SAPBR=3,1,\"USER\",\"" + String(gprsUser) + "\"");
    sendCommand("AT+SAPBR=3,1,\"PWD\",\"" + String(gprsPass) + "\"");
    sendCommand("AT+SAPBR=1,1");
    sendCommand("AT+SAPBR=2,1");
}

bool Sim800Updater::initSpiffs(void) {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    if (SPIFFS.format()) {
        Serial.println("SPIFFS formatted successfully!");
        return true;
    } else {
        Serial.println("SPIFFS format failed!");
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
        Serial.println("Failed to open file for writing");
        return false;
    }

    Serial.println("Initializing SIM800 FTP...");
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
        //Serial.println("AT Command: AT+FTPGET=2,256");// Запрос данных
        Serial1.println("AT+FTPGET=2,256");
        delay(150);//50 ms minimum delay
        String response = "";
        while (Serial1.available()) {
            char c = Serial1.read();
            response += c;
        }
        //Serial.println(response);
        if (response.indexOf("\r\n+FTPGET: 1,0") != -1) {
            break; // Передача завершена
        }
        
        int startIndex = response.indexOf("+FTPGET: 2,", 2);
        //Serial.printf("\nstartIndex: %d\n", startIndex);        
        if (startIndex != -1) {
            startIndex += 11; // Пропускаем "+FTPGET: 2,"            
            int endIndex = response.indexOf("\r\n", startIndex);
            int availableBytes = response.substring(startIndex, endIndex).toInt();
            //Serial.printf("availableBytes: %d\n", availableBytes);
            startIndex = endIndex + 2; // Пропускаем "\r\n"
            if (availableBytes > 0) {
                char c;                
                for (int i = startIndex; i < response.length(); i++) {
                    if (i + 1 < response.length() && strncmp(&response[i], "\r\nOK", 4) == 0) {
                        break;
                    }
                    c= response.charAt(i);
                    file.write(response[i]);                    
                    Serial.printf("%c", c);
                    totalBytes++;
                }
            }            
        }
    }
    file.close();
    sendCommand("AT+FTPCLOSE", 5000);  // Закрытие FTP-сессии
    Serial.println("Download complete!");
    Serial.print("Total bytes downloaded: ");
    Serial.println(totalBytes);
    return true;
}

void Sim800Updater::updateFirmwareViaGPRS(){    
    if (!initSpiffs()) return;        
    if(downloadFirmware()){
        // Fetch CRC32 value of the firmware from the server
        uint32_t serverCRC = fetchCRC32(); 
        uint32_t fileCRC32 = calculateFileCRC32(LOCAL_FILE);
        Serial.printf("CRC32 файла: %08X\n", fileCRC32);   
        if (fileCRC32 == serverCRC) {
            Serial.println("CRC32 совпадает!");
            performFirmwareUpdate();
        } else {
            Serial.println("Ошибка: CRC32 не совпадает!");
            return;
        } 
    } else {
        Serial.println("Ошибка загрузки файла прошивки!");
        return;
    }  
}

bool Sim800Updater::checkForUpdates(){
    return false;
} 