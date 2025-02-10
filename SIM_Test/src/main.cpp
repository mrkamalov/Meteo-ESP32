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
#include "FirmwareUpdater.h"
#include "GSM.h"
#include <HardwareSerial.h>

const char* firmware_file_path = "/firmware.bin"; // Path for saving the firmware
const uint32_t expected_crc32 = 0x6f50d767; // Example CRC32 checksum

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);
  
  // Initialize SPIFFS
  if (!initSPIFFS()) return;

  initGSMModem();
}

void loop() {
  
  connectToGPRS();
  uint16_t partsNum = getPartsNumber();
  String body = loadFirmware();
  // Perform firmware update
  performFirmwareUpdate(body, firmware_file_path, expected_crc32);
  disconnectGPRS();

  // Do nothing forevermore
  while (true) { delay(1000); }  
}
