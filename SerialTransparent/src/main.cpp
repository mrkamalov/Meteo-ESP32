#include <HardwareSerial.h>
#include "SPIFFS.h"

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8

const char* FTP_SERVER = "ftp.drivehq.com";
const char* FTP_USER = "MeteoESP";
const char* FTP_PASS = "Rsu!wCzvzE52FS7";
const char* FILE_NAME = "firmware.bin";  // Загружаемый файл
const char* LOCAL_FILE = "/firmware.bin";  // Сохранение в SPIFFS
const char apn[] = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";

void sendCommand(String command, int delayTime = 2000) {
    Serial.println("AT Command: " + command);
    Serial1.println(command);
    delay(delayTime);
    while (Serial1.available()) {
        Serial.write(Serial1.read());  // Отладочный вывод
    }
}

void waitForResponse(const String& expectedResponse, int timeout = 5000) {
    unsigned long startTime = millis();
    String response = "";
    while (millis() - startTime < timeout) {
        while (Serial1.available()) {
            char c = Serial1.read();
            Serial.write(c);  // Отладочный вывод
            response += c;
            if (response.indexOf(expectedResponse) != -1) {
                return;
            }
        }
    }
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
    delay(6000);    
}

void loop() {    
    // Передача из Serial в Serial1
    while (Serial.available()) {
        Serial1.write(Serial.read());
    }    
    // Передача из Serial1 в Serial
    while (Serial1.available()) {
        Serial.write(Serial1.read());
    }
}
