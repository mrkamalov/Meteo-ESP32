#include <HardwareSerial.h>

// Определите пины для подключения модема
#define UART2_TX_PIN 17
#define UART2_RX_PIN 16
#define UART_BAUDRATE 9600

void setup() {
    // Set console baud rate
    Serial.begin(UART_BAUDRATE);
    Serial2.begin(UART_BAUDRATE, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
    delay(10);
    Serial.println("UART bridge started");      
}

void loop() {    
    // Передача из Serial в Serial1
    while (Serial.available()) {
        Serial2.write(Serial.read());
    }    
    // Передача из Serial1 в Serial
    while (Serial2.available()) {
        Serial.write(Serial2.read());
    }
}
