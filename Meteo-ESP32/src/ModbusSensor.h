#ifndef MODBUS_SENSOR_H
#define MODBUS_SENSOR_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include "deviceConfig.h"

class ModbusSensor {
public:
    ModbusSensor(uint8_t slaveId);

    void begin();

    // Чтение одного регистра
    bool readRegister(uint16_t reg, uint16_t &result);
    // Чтение нескольких регистров
    bool readRegisters(uint16_t startReg, uint8_t count, uint16_t* results);
    bool readInputRegisters(uint16_t startReg, uint8_t count, uint16_t* results);
    bool readSensorData(SensorData &data, bool swapWords);

private:
    static void preTransmission();
    static void postTransmission();

    static SoftwareSerial* _softSerial;
    ModbusMaster _node;
    uint8_t _slaveId;
};

#endif // MODBUS_SENSOR_H
