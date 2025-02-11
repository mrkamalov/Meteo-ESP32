#ifndef GSM_H
#define GSM_H

#include <Arduino.h>

void initGSMModem(void);
void connectToGPRS(void);
void disconnectGPRS(void);
String loadFirmwarePart(int partNumber);
void getConfigData(uint32_t &partsCount, uint32_t &crc, uint32_t &totalSize);
#endif // GSM_H