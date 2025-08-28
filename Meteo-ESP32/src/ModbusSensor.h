#ifndef MODBUS_SENSOR_H
#define MODBUS_SENSOR_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include "deviceConfig.h"

enum SensorType {
  SE_1,
  OTHER
};

class ModbusSensor {
public:    
    void begin(uint8_t slaveId, SensorType type);

    // Чтение одного регистра
    bool readRegister(uint16_t reg, uint16_t &result);
    // Чтение нескольких регистров
    bool readRegisters(uint16_t startReg, uint8_t count, uint16_t* results);
    bool readInputRegisters(uint16_t startReg, uint8_t count, uint16_t* results);
    bool readSensorData(SensorData &data);
    bool readEcoSensorData(SensorData &data);

private:
    static void preTransmission();
    static void postTransmission();
    float toFloat(uint16_t hi, uint16_t lo);

    static SoftwareSerial* _softSerial;
    ModbusMaster _node;
    SensorType sensorType;
};

#endif // MODBUS_SENSOR_H
