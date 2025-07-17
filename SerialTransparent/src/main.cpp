#include <HardwareSerial.h>
#include "SPIFFS.h"

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_GSM_EN_PIN 19
#define MODEM_PWRKEY_PIN 37

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
    
    // Включение питания модема
    pinMode(MODEM_GSM_EN_PIN, OUTPUT);    
    digitalWrite(MODEM_GSM_EN_PIN, HIGH);
    Serial.println("Waiting for modem to power up...");
    delay(2000);
    pinMode(MODEM_PWRKEY_PIN, OUTPUT);
    digitalWrite(MODEM_PWRKEY_PIN, HIGH);
    Serial.println("PWR KEY LOW");
    delay(2000);
    digitalWrite(MODEM_PWRKEY_PIN, LOW);  // Включаем питание модема
    Serial.println("PWR KEY HIGH");

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
