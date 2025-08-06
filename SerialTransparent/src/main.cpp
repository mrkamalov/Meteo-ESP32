#include <HardwareSerial.h>
#include "SPIFFS.h"

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_GSM_EN_PIN 19
#define MODEM_PWRKEY_PIN 37
#define MODEM_STATUS_PIN 38

// Select your modem:
#define TINY_GSM_MODEM_SIM800
// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
#define SerialAT Serial1
// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

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
    Serial.begin(921600);//115200);
    
    // Включение питания модема
  SerialMon.println("Modem power up delay");  
  pinMode(MODEM_GSM_EN_PIN, OUTPUT);
  digitalWrite(MODEM_GSM_EN_PIN, LOW);
  delay(3200);
  digitalWrite(MODEM_GSM_EN_PIN, HIGH);
  SerialMon.println("Waiting for modem to power up...");
  delay(500);
  pinMode(MODEM_PWRKEY_PIN, OUTPUT);
  digitalWrite(MODEM_PWRKEY_PIN, HIGH);
  SerialMon.println("PWR KEY LOW");
  delay(1500);  
  digitalWrite(MODEM_PWRKEY_PIN, LOW);  // Включаем питание модема
  SerialMon.println("PWR KEY HIGH");
  SerialMon.println("Wait...");
  delay(2000);
  pinMode(MODEM_STATUS_PIN, INPUT); // Set status pin as input
  //read status pin
  if (digitalRead(MODEM_STATUS_PIN) != HIGH) delay(2000);

    // Set GSM module baud rate
    Serial1.begin(921600);//, SERIAL_8N1, MODEM_RX, MODEM_TX);    //115200
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
