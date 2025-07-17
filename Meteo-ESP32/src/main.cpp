#include <HardwareSerial.h>
#include "TransmissionManager.h"
#include "SerialMon.h"

static TransmissionManager transmissionManager;

void sendAT(const String& command, uint32_t timeout = 2000) {
  Serial.println("Sending AT command: " + command);
  SerialAT.println(command);
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (SerialAT.available()) {
      Serial.write(SerialAT.read()); // Выводим ответ модема в монитор порта      
    }
  }
  Serial.println(); // Переход на новую строку
}

void setup() {
    // Set console baud rate
    Serial.begin(9600); 
    SerialMon.begin(9600, SERIAL_8N1, 41, 42);  // RX=41, TX=42   
    Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    //meteoPortal.begin();
    //simUpdater.init();
    //simClient.begin();  
    // SerialMon.println("Starting Meteo Station...");
    transmissionManager.begin();

    // Включение питания модема
    // pinMode(MODEM_GSM_EN_PIN, OUTPUT);    
    // digitalWrite(MODEM_GSM_EN_PIN, HIGH);
    // Serial.println("Waiting for modem to power up...");
    // delay(2000);
    // pinMode(MODEM_PWRKEY_PIN, OUTPUT);
    // digitalWrite(MODEM_PWRKEY_PIN, HIGH);
    // Serial.println("PWR KEY LOW");
    // delay(2000);
    // digitalWrite(MODEM_PWRKEY_PIN, LOW);  // Включаем питание модема
    // Serial.println("PWR KEY HIGH");

    // sendAT("AT");                  // Проверка связи
    // sendAT("ATE0");                // Отключить эхо
    // sendAT("AT+CPIN?");            // Проверка SIM-карты
    // sendAT("AT+CSQ");              // Уровень сигнала
    // sendAT("AT+CREG?");            // Регистрация в сети
    // sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\""); // Настройка типа соединения
    // sendAT("AT+SAPBR=3,1,\"APN\",\"internet.altel.kz\"");  // Оператор
    // sendAT("AT+SAPBR=1,1");        // Открыть соединение
    // sendAT("AT+SAPBR=2,1");        // Получить информацию о соединении
}

void loop() {    
    while (true) {
        //delay(1000);
        //if(simUpdater.checkForUpdates()) simUpdater.updateFirmwareViaGPRS();// Объединить в один метод
        //meteoPortal.loop();
        //simClient.loop();  

        transmissionManager.loop();      
    }  
}


/*
#include <WiFi.h>
#include <AsyncTCP.h>

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

const char* ssid = "SG_GUEST";
const char* password = "";

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  ElegantOTA.loop();
}

*/