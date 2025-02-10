#ifndef GSM_H
#define GSM_H

#include <Arduino.h>

void initGSMModem(void);
void connectToGPRS(void);
void disconnectGPRS(void);
String loadFirmware(void);
uint16_t getPartsNumber();
#endif // GSM_H