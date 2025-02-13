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
//#include "esp_system.h"

void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);
  // Serial.print("Свободно памяти: ");
  // Serial.print(heap_caps_get_free_size(0));
  // Serial.println(" байт");
  // // Мин. свободный блок памяти
  // Serial.print("Мин. свободный блок: ");
  // Serial.print(heap_caps_get_minimum_free_size(0));
  // Serial.println(" байт");
  // // Макс. доступный блок
  // Serial.print("Макс. доступный блок: ");
  // Serial.print(heap_caps_get_largest_free_block(0));
  // Serial.println(" байт");
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
