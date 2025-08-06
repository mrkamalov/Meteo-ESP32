// #include <HardwareSerial.h>
// #include "TransmissionManager.h"
// #include "SerialMon.h"

// static TransmissionManager transmissionManager;

// void sendAT(const String& command, uint32_t timeout = 2000) {
//   Serial.println("Sending AT command: " + command);
//   SerialAT.println(command);
//   unsigned long start = millis();
//   while (millis() - start < timeout) {
//     while (SerialAT.available()) {
//       Serial.write(SerialAT.read()); // Выводим ответ модема в монитор порта      
//     }
//   }
//   Serial.println(); // Переход на новую строку
// }

// void setup() {
//   // Set console baud rate
//   Serial.begin(9600); 
//   SerialMon.begin(9600, SERIAL_8N1, 41, 42);  // RX=41, TX=42   
//   Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
//   //meteoPortal.begin();
//   //simUpdater.init();
//   //simClient.begin();  
//   // SerialMon.println("Starting Meteo Station...");

//   transmissionManager.begin();

//   //SET DEFAULT PIN function
//   // pinMode(SET_DEFAULTS_PIN, INPUT);  
// }

// void loop() {    
//   while (true) {
//     //delay(1000);
//     //if(simUpdater.checkForUpdates()) simUpdater.updateFirmwareViaGPRS();// Объединить в один метод
//     //meteoPortal.loop();
//     //simClient.loop();  

//     transmissionManager.loop(); 

//     //SET DEFAULT PIN check
//     // delay(1000); // Задержка для предотвращения слишком частого цикла    
//     // int state = digitalRead(SET_DEFAULTS_PIN);  // читаем 0 или 1  
//     // if (state == HIGH) {
//     //   SerialMon.println("Вход ВКЛЮЧЕН (логическая 1)");
//     // } else {
//     //   SerialMon.println("Вход ВЫКЛЮЧЕН (логический 0)");
//     // }
//   }  
// }


// #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <HTTPClient.h>

// void checkExternalPower();
// void sendPowerLossAlert();
// void sendTelegramMessage(const String& message);

// const char* ssid = "SG_RND";
// const char* password = "korkemwifi";

// String telegramBotToken = "8227335448:AAE1u8sB7tDNIDPtasvnVEe1O6uRkYtB0NQ";
// String chatId = "-4622112875"; // Ваш chat ID
// WiFiClientSecure secured_client;

// // ==== Root CA для api.telegram.org ====
// const char TELEGRAM_CERT[] PROGMEM = R"EOF(
// -----BEGIN CERTIFICATE-----
// MIIDxTCCAq2gAwIBAgIQRmU2ZBIykkBHzSm6mRypxzANBgkqhkiG9w0BAQsFADA/
// MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xGDAWBgNVBAMT
// D1Jvb3QgQ0EgUm9vdCAtIEcyMB4XDTIwMDYyNTAwMDAwMFoXDTMwMDYyNDIzNTk1
// OVowPzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRgwFgYD
// VQQDEw9Sb290IENBIFJvb3QgLSBHMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
// AQoCggEBAKypx2vwEjh23UNMpddT+2sW0kTLaCkqn5Z8S0S3Vfhz9R+oTBtC1c6C
// GZnKr6KPyHOvW+IEdHTrnDUudY1zM04tWKh5+AqqcDKZykB1WUNvbxi5zsxgJK2/
// g04QLjHqqE/5bN0msYAXIbFTleYhA7aMne9wuf3ls7n1FUyQrrKJmwhXWf3KTz1A
// VyeC1VGKRDcrEX6FxN2dXrPYAz5+0I0z+od8k/RAOU/yU35g8sbQ0BYII40di6KI
// vF/jbpDFH9zAyA+3gswHDb6uB2SVuOiG7csV0rOW7jQ+zY7K1o1uNpxvQiluzpXL
// f75E+f4Z9M6dFbTD+kQoh9ev9X49wscCAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB
// /zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYEFNg+JNnx18t1pHABH6Y/2LG2F8qw
// MA0GCSqGSIb3DQEBCwUAA4IBAQCZpEaj2lzF+enKnnOgzmIHmBd5dtn8rYJQ2v4f
// C3y5KP8v8PZKxDSHLVYRIlnJrped1IovnHgwlHGawEq+y3OC/YLXTr4Wr9L5xRcm
// F8UOB7hZPH/LW0OX2uUVksNzx8dx4qPuIgz8fcwTN82ZMKmDG4GJHzFg3Ajc6NNC
// pUg/6QMQ6MCTQ2xwbGHptRNZB+Yh8Ejua+ZC3WnSNUFjL0kJ4kgE3AQUrdRr+88j
// cZMcG2JSe2xJJHptNwacrkT4VLT2KT+6U6FqVuwZYaUysJ/k6FOLwObAHTkKQ+39
// rMxuF8k+nMB0Igf4lhkSNuL6Q8v2sG9FL4+bQMbsQzH/66pF
// -----END CERTIFICATE-----
// )EOF";

// // Пин для мониторинга питания
// #define POWER_PIN 16
// #define POWER_LOSS_THRESHOLD 1500  // мВ (ниже считаем что питание пропало)

// bool powerLost = false;
// bool prevPowerLost = false;

// // === Инициализация ===
// void setup() {
//   Serial.begin(9600);//115200);

//   // ADC настройки
//   analogSetAttenuation(ADC_11db);       // для измерения до ~3.3 В
//   analogReadResolution(12);             // 0–4095
//   pinMode(POWER_PIN, INPUT);

//   // Подключение к WiFi
//   WiFi.begin(ssid, password);
//   Serial.print("Подключение к WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nWiFi подключено!");
//   Serial.print("IP: ");
//   Serial.println(WiFi.localIP());
// }

// // === Основной цикл ===
// void loop() {
//   checkExternalPower();
//   delay(2000);  // проверка каждые 2 сек
// }

// // === Проверка внешнего питания ===
// void checkExternalPower() {
//   static unsigned long powerLossTime = 0;
//   int raw = analogRead(POWER_PIN);
//   float voltage = (raw / 4095.0) * 3300; // мВ
//   Serial.printf("Напряжение на IO16: %.2f мВ\n", voltage);
//   if(powerLost){    
//     Serial.println("Время работы без питания (мс): " + String(millis()-powerLossTime));
//   }

//   prevPowerLost = powerLost;
//   powerLost = voltage < POWER_LOSS_THRESHOLD;

//   if (powerLost && !prevPowerLost) {
//     Serial.println("⚠️ Обнаружено пропадание внешнего питания!");
//     powerLossTime = millis(); // Запоминаем время потери питания
//     sendPowerLossAlert();
//   } else if (!powerLost && prevPowerLost) {
//     Serial.println("✅ Внешнее питание восстановлено.");
//   }
// }

// // === Отправка уведомления в Telegram ===
// void sendPowerLossAlert() {
//   String message = "⚠️ Обнаружено пропадание внешнего питания устройства!";
//   sendTelegramMessage(message);
// }

// void sendTelegramMessage(const String& message) {
//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("WiFi не подключен. Не удалось отправить сообщение.");
//     return;
//   }
//   secured_client.setCACert(TELEGRAM_CERT);

//   HTTPClient http;
//   String url = "https://api.telegram.org/bot" + telegramBotToken + "/sendMessage";

//   http.begin(url);
//   http.addHeader("Content-Type", "application/x-www-form-urlencoded");

//   String body = "chat_id=" + chatId + "&text=" + message;
//   int httpResponseCode = http.POST(body);

//   if (httpResponseCode > 0) {
//     String response = http.getString();
//     Serial.println("Сообщение отправлено:");
//     Serial.println(response);
//   } else {
//     Serial.printf("Ошибка при отправке: %d\n", httpResponseCode);
//   }

//   http.end();
// }

#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_GSM_EN_PIN 19
#define MODEM_PWRKEY_PIN 37
#define MODEM_STATUS_PIN 38

#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""

const char apn[]      = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char server[]   = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int  port       = 443;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm        modem(SerialAT);
TinyGsmClientSecure client(modem);
HttpClient          http(client, server, port);

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

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

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(10000L, true)) {
    SerialMon.println(" fail");    
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }
  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }
}

void loop() {
  SerialMon.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive();  // Currently, this is needed for HTTPS
  int err = http.get(resource);
  if (err != 0) {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  SerialMon.print(F("Body length is: "));
  SerialMon.println(body.length());

  // Shutdown

  http.stop();
  SerialMon.println(F("Server disconnected"));

  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));

  // Do nothing forevermore
  while (true) { delay(1000); }
}