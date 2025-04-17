#include <HardwareSerial.h>
#include "sim800Updater.h"
#include "MeteoConfigPortal.h"
#include "Sim868Client.h"

#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8
#define LED_PIN 39
#define SerialMon Serial
#define SerialAT Serial1

static Sim800Updater simUpdater;
static MeteoConfigPortal meteoPortal;
static Sim868Client simClient(SerialAT, SerialMon, MODEM_POWER_PIN, LED_PIN);

void setup() {
    // Set console baud rate
    Serial.begin(115200);    
    Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    //meteoPortal.begin();
    //simUpdater.init();
    simClient.begin();
}

void loop() {    
    while (true) {
        //delay(1000);
        //if(simUpdater.checkForUpdates()) simUpdater.updateFirmwareViaGPRS();// Объединить в один метод
        //meteoPortal.loop();
        simClient.loop();
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