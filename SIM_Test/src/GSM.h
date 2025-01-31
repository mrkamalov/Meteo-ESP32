#ifndef GSM_H
#define GSM_H

#include <Arduino.h>

void initGSMModem(void);
void connectToGPRS(void);
void disconnectGPRS(void);
String loadFirmware(void);
#endif // GSM_H