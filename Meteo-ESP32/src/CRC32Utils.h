#ifndef CRC32_UTILS_H
#define CRC32_UTILS_H

#include <Arduino.h>

void initCRCTable();
uint32_t updateCRC32(uint32_t crc, uint8_t data);

#endif
