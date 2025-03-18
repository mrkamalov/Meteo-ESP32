#include <HardwareSerial.h>
#include "SPIFFS.h"
#include "Update.h"

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8
#define BUFFER_SIZE 1024  // Размер буфера для чтения

const char* FTP_SERVER = "ap-southeast-1.sftpcloud.io";
const char* FTP_USER = "2244dd5bba3741a0a3bf222a33e5798a";
const char* FTP_PASS = "I1XaPfmML2rjzLSK273dYiGvoFONfCR5";
const char* FILE_NAME = "firmware.bin";  // Загружаемый файл
const char* LOCAL_FILE = "/firmware.bin";  // Сохранение в SPIFFS
const char apn[] = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";
uint32_t crcTable[256];
const char* url = "https://mrkamalov.github.io/Meteo-ESP32/firmware/config.txt";  // URL файла для загрузки

void performFirmwareUpdate() {
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
void initCRCTable() {
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
uint32_t updateCRC32(uint32_t crc, uint8_t data) {
    return (crc >> 8) ^ crcTable[(crc ^ data) & 0xFF];
}

// Функция расчета CRC32 для файла в SPIFFS
uint32_t calculateFileCRC32(const char* filePath) {
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

void sendCommand(String command, int delayTime = 3000) {
    Serial.println("AT Command: " + command);
    Serial1.println(command);
    delay(delayTime);
    while (Serial1.available()) {
        Serial.write(Serial1.read());  // Отладочный вывод
    }
}

bool waitForResponse(const String& expectedResponse, int timeout = 5000) {
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

void setup() {
    // Set console baud rate
    Serial.begin(115200);
    delay(10);
    // Включение питания модема
    pinMode(MODEM_POWER_PIN, OUTPUT);
    digitalWrite(MODEM_POWER_PIN, HIGH);
    Serial.println("Wait...");

    // Set GSM module baud rate
    Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    initCRCTable(); // Инициализация таблицы CRC32
    if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
    }
    if (SPIFFS.format()) {
        Serial.println("SPIFFS formatted successfully!");
    } else {
        Serial.println("SPIFFS format failed!");
    }
    delay(6000);
    Serial.println("Initializing SIM800 FTP...");
    sendCommand("AT");
    sendCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    sendCommand("AT+SAPBR=3,1,\"APN\",\"" + String(apn) + "\"");
    sendCommand("AT+SAPBR=3,1,\"USER\",\"" + String(gprsUser) + "\"");
    sendCommand("AT+SAPBR=3,1,\"PWD\",\"" + String(gprsPass) + "\"");
    sendCommand("AT+SAPBR=1,1");
    sendCommand("AT+SAPBR=2,1");
    sendCommand("AT+HTTPINIT");  // Инициализация HTTP
    sendCommand("AT+HTTPPARA=\"CID\",1");  // Выбор профиля GPRS
    sendCommand("AT+HTTPPARA=\"URL\",\"" + String(url) + "\"");
    sendCommand("AT+HTTPSSL=1");  // Включить SSL
    sendCommand("AT+HTTPACTION=0");  // Отправка HTTP GET
    if (!waitForResponse("+HTTPACTION: 0,200", 10000)) {
        Serial.println("HTTP GET Failed");
        return;
    }
    Serial1.println("AT+HTTPREAD");
    delay(150);//50 ms minimum delay
    String response = "";
    while (Serial1.available()) {
        char c = Serial1.read();
        response += c;
    }
    sendCommand("AT+HTTPTERM");  // Завершение HTTP-сессии

    sendCommand("AT+FTPCID=1");
    sendCommand("AT+FTPSERV=\"" + String(FTP_SERVER) + "\"");
    sendCommand("AT+FTPUN=\"" + String(FTP_USER) + "\"");
    sendCommand("AT+FTPPW=\"" + String(FTP_PASS) + "\"");
    sendCommand("AT+FTPTYPE=I");
    sendCommand("AT+FTPGETNAME=\"" + String(FILE_NAME) + "\"");
    sendCommand("AT+FTPGETPATH=\"/\"");

    File file = SPIFFS.open(LOCAL_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

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
    Serial.printf("CRC32 файла: %08X\n", calculateFileCRC32(LOCAL_FILE));
    performFirmwareUpdate();
}

void loop() {    
  // Do nothing forevermore
  while (true) { delay(1000); }  
}
