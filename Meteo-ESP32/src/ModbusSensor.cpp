#include "ModbusSensor.h"

SoftwareSerial* ModbusSensor::_softSerial = nullptr;

ModbusSensor::ModbusSensor(uint8_t slaveId)
    : _slaveId(slaveId) {}

void ModbusSensor::begin() {
    // Настройка пина RE/DE как выход
    pinMode(MODBUS_REDE_PIN, OUTPUT);
    digitalWrite(MODBUS_REDE_PIN, LOW); // Начинаем в режиме приёма
    // Создаём программный UART
    _softSerial = new SoftwareSerial(MODBUS_RX_PIN, MODBUS_TX_PIN, false);
    _softSerial->begin(MODBUS_BAUDRATE);    

    // Настройка ModbusMaster
    _node.begin(_slaveId, *_softSerial);
    _node.preTransmission(preTransmission);
    _node.postTransmission(postTransmission);
}

bool ModbusSensor::readRegister(uint16_t reg, uint16_t &result) {
    uint8_t ec = _node.readHoldingRegisters(reg, 1);
    if (ec == _node.ku8MBSuccess) {
        result = _node.getResponseBuffer(0);
        return true;
    }
    return false;
}

bool ModbusSensor::readRegisters(uint16_t startReg, uint8_t count, uint16_t* results) {
    uint8_t ec = _node.readHoldingRegisters(startReg, count);
    if (ec == _node.ku8MBSuccess) {
        for (uint8_t i = 0; i < count; i++) {
            results[i] = _node.getResponseBuffer(i);
        }
        return true;
    }
    return false;
}

void ModbusSensor::preTransmission() {
    digitalWrite(MODBUS_REDE_PIN, HIGH); // Включаем передачу
}

void ModbusSensor::postTransmission() {
    digitalWrite(MODBUS_REDE_PIN, LOW);  // Включаем приём
}

bool ModbusSensor::readInputRegisters(uint16_t startReg, uint8_t count, uint16_t* results) {
    uint8_t ec = _node.readInputRegisters(startReg, count);
    if (ec == _node.ku8MBSuccess) {
        for (uint8_t i = 0; i < count; i++) {
            results[i] = _node.getResponseBuffer(i);
        }
        return true;
    }
    return false;
}

bool ModbusSensor::readSensorData(SensorData &data, bool swapWords) {
    const uint16_t START_REG = 0;
    const uint8_t WORDS_NEEDED = 18;
    uint16_t buffer[WORDS_NEEDED];

    if (!readInputRegisters(START_REG, WORDS_NEEDED, buffer)) {
        return false;
    }

    auto getFloat = [&](int idx) -> float {
        union {
            uint32_t u;
            float f;
        } val;

        if (swapWords) {
            // LoWord, HiWord
            val.u = (static_cast<uint32_t>(buffer[idx + 1]) << 16) | buffer[idx];
        } else {
            // HiWord, LoWord
            val.u = (static_cast<uint32_t>(buffer[idx]) << 16) | buffer[idx + 1];
        }

        return val.f;
    };

    data.gas1         = getFloat(0);
    data.gas2         = getFloat(2);
    data.gas3         = getFloat(4);
    data.gas4         = getFloat(6);
    data.internalTemp = getFloat(8);
    data.pm25         = getFloat(10);
    data.pm10         = getFloat(12);
    data.externalTemp = getFloat(14);
    data.humidity     = getFloat(16);

    return true;
}