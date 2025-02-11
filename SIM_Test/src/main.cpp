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
  performFirmwareUpdate();  
  disconnectGPRS();

  // Do nothing forevermore
  while (true) { delay(1000); }  
}
