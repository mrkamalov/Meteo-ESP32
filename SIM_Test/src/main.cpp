/**************************************************************
 *
 * For this example, you need to install CRC32 library:
 *   https://github.com/bakercp/CRC32
 *   or from http://librarymanager/all#CRC32+checksum
 *
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *
 * ATTENTION! Downloading big files requires of knowledge of
 * the TinyGSM internals and some modem specifics,
 * so this is for more experienced developers.
 *
 **************************************************************/
#include <SPIFFS.h>
#include <Update.h>
// Select your modem:
#define TINY_GSM_MODEM_SIM800
#include <HardwareSerial.h>
// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
#define SerialAT Serial1

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 1024
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// #define LOGGING  // <- Logging is for the HTTP library

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "internet.altel.kz";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// Server details
const char server[] = "mrkamalov.github.io";//https://
const int  port     = 443;//80;
const char* firmware_file_path = "/firmware.bin"; // Path for saving the firmware
uint32_t expected_crc32 = 0x6f50d767; // Example CRC32 checksum

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <CRC32.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

const char resource[]    = "/Meteo-ESP32/test_50k.bin";
uint32_t   knownCRC32    = 0x6f50d767;
uint32_t   knownFileSize = 1024;  // In case server does not send it

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

TinyGsmClientSecure client(modem);
HttpClient          http(client, server, port);
bool saveFirmwareToSPIFFS(const String& data);
bool verifyFirmwareChecksum(const char* filePath, uint32_t expectedCRC);
bool writeFirmwareToOTA(const char* filePath);
// Определите пины для подключения модема
#define MODEM_RX 18
#define MODEM_TX 17
#define MODEM_POWER_PIN 8
void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed");
    return;
  }
  Serial.println("SPIFFS initialized successfully!");

  // Включение питания модема
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);

  SerialMon.println("Wait...");

  // Set GSM module baud rate
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif
}

void printPercent(uint32_t readLength, uint32_t contentLength) {
  // If we know the total length
  if (contentLength != (uint32_t)-1) {
    SerialMon.print("\r ");
    SerialMon.print((100.0 * readLength) / contentLength);
    SerialMon.print('%');
  } else {
    SerialMon.println(readLength);
  }
}

void loop() {
#if TINY_GSM_USE_WIFI
  // Wifi connection parameters must be set before waiting for the network
  SerialMon.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

#if TINY_GSM_USE_GPRS
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
#endif

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

  // Save firmware to SPIFFS
  if (saveFirmwareToSPIFFS(body)) {
    Serial.println("Firmware saved to SPIFFS.");

    // Verify checksum
    if (verifyFirmwareChecksum(firmware_file_path, expected_crc32)) {
      Serial.println("Firmware checksum is valid.");

      // Write firmware to OTA partition and update
      if (writeFirmwareToOTA(firmware_file_path)) {
        Serial.println("Firmware updated successfully. Restarting...");
        ESP.restart();
      } else {
        Serial.println("Firmware update failed.");
      }
    } else {
      Serial.println("Error: Checksum mismatch.");
    }
  } else {
    Serial.println("Error saving firmware to SPIFFS.");
  }

  // Shutdown
  http.stop();
  SerialMon.println(F("Server disconnected"));

#if TINY_GSM_USE_WIFI
  modem.networkDisconnect();
  SerialMon.println(F("WiFi disconnected"));
#endif
#if TINY_GSM_USE_GPRS
  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));
#endif

  // Do nothing forevermore
  while (true) { delay(1000); }  
}

// Saves the firmware string to a SPIFFS file
bool saveFirmwareToSPIFFS(const String& data) {  
  File firmwareFile = SPIFFS.open(firmware_file_path, FILE_WRITE);
  if (!firmwareFile) {
    Serial.println("Failed to open file for writing.");
    return false;
  }

  size_t bytesWritten = firmwareFile.print(data);
  firmwareFile.close();

  if (bytesWritten == data.length()) {
    Serial.printf("Successfully wrote %d bytes.\n", bytesWritten);
    return true;
  } else {
    Serial.printf("Write error: wrote %d bytes out of %d.\n", bytesWritten, data.length());
    return false;
  }
}

// Verifies the checksum of the firmware file
bool verifyFirmwareChecksum(const char* filePath, uint32_t expectedCRC) {
  File firmwareFile = SPIFFS.open(filePath, FILE_READ);
  if (!firmwareFile) {
    Serial.println("Failed to open file for checksum verification.");
    return false;
  }

  CRC32 crc;

  uint8_t buffer[128];
  while (firmwareFile.available()) {
    size_t bytesRead = firmwareFile.read(buffer, sizeof(buffer));
    crc.update(buffer, bytesRead);
  }

  firmwareFile.close();
  uint32_t actualCRC = crc.finalize();
  Serial.printf("File checksum: 0x%08X\n", actualCRC);

  return actualCRC == expectedCRC;
}

// Writes firmware from SPIFFS to the OTA partition
bool writeFirmwareToOTA(const char* filePath) {
  File firmwareFile = SPIFFS.open(filePath, FILE_READ);
  if (!firmwareFile) {
    Serial.println("Failed to open file for OTA update.");
    return false;
  }

  size_t firmwareSize = firmwareFile.size();
  Serial.printf("Firmware size: %d bytes\n", firmwareSize);

  if (!Update.begin(firmwareSize)) {
    Serial.println("Error: Not enough space for the update.");
    firmwareFile.close();
    return false;
  }

  uint8_t buffer[128];
  size_t totalBytes = 0;

  while (firmwareFile.available()) {
    size_t bytesRead = firmwareFile.read(buffer, sizeof(buffer));
    if (Update.write(buffer, bytesRead) != bytesRead) {
      Serial.println("Error writing data to OTA.");
      Update.abort();
      firmwareFile.close();
      return false;
    }
    totalBytes += bytesRead;
  }

  firmwareFile.close();

  if (Update.end()) {
    if (Update.isFinished()) {
      Serial.println("OTA update completed successfully!");
      return true;
    } else {
      Serial.println("Error: Update not finished.");
    }
  } else {
    Serial.printf("Error completing update: %s\n", Update.errorString());
  }

  return false;
}