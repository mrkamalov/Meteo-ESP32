#include <HardwareSerial.h>
#include "SPIFFS.h"

// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8
#define SerialAT Serial1

const char* FTP_SERVER = "ftp.example.com";
const char* FTP_USER = "your_username";
const char* FTP_PASS = "your_password";
const char* FILE_NAME = "bigfile.bin";  // Загружаемый файл
const char* LOCAL_FILE = "/bigfile.bin";  // Сохранение в SPIFFS

void sendCommand(String command, int delayTime = 2000) {
    Serial.println("AT Command: " + command);
    Serial1.println(command);
    delay(delayTime);
    while (Serial1.available()) {
        Serial.write(Serial1.read());  // Отладочный вывод
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
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(6000);
  if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    Serial.println("Initializing SIM800 FTP...");

    sendCommand("AT");
    sendCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    sendCommand("AT+SAPBR=3,1,\"APN\",\"internet\"");
    sendCommand("AT+SAPBR=1,1");
    sendCommand("AT+SAPBR=2,1");

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

    int totalBytes = 0;
    while (true) {
        sendCommand("AT+FTPGET=2,1024");  // Читаем 1024 байта

        delay(1000);  // Даем время SIM800
        if (!Serial1.available()) break;  // Если данных нет — файл загружен

        while (Serial1.available()) {
            char c = Serial1.read();
            file.write(c);
            totalBytes++;
        }
        Serial.print("Downloaded: ");
        Serial.println(totalBytes);
    }

    file.close();
    Serial.println("Download complete!");
}

void loop() {    
  // Do nothing forevermore
  while (true) { delay(1000); }  
}
