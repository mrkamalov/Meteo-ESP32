#include "CRC32Utils.h"

static uint32_t crc_table[256];

void initCRCTable() {
    const uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crc_table[i] = crc;
    }
}

uint32_t updateCRC32(uint32_t crc, uint8_t data) {
    return (crc >> 8) ^ crc_table[(crc ^ data) & 0xFF];
}