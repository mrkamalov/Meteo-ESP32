#include "ModbusSensor.h"

SoftwareSerial* ModbusSensor::_softSerial = nullptr;

void ModbusSensor::begin(uint8_t slaveId, SensorType type) {
    sensorType = type;
    // Настройка пина RE/DE как выход
    pinMode(MODBUS_REDE_PIN, OUTPUT);
    digitalWrite(MODBUS_REDE_PIN, LOW); // Начинаем в режиме приёма
    // Создаём программный UART
    //_softSerial = new SoftwareSerial(MODBUS_RX_PIN, MODBUS_TX_PIN, false);
    //_softSerial->begin(MODBUS_BAUDRATE);    

    // Настройка ModbusMaster
    _node.begin(slaveId, Serial);//*_softSerial);
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

float ModbusSensor::toFloat(uint16_t hi, uint16_t lo) {
    uint32_t raw = (static_cast<uint32_t>(hi) << 16) | lo;
    float f;
    memcpy(&f, &raw, sizeof(f));
    return f;
}

bool ModbusSensor::readEcoSensorData(SensorData &data) {    
    const uint16_t START_REG = 0;
    const uint8_t WORDS_NEEDED = 18;
    uint16_t buffer[WORDS_NEEDED];

    if (!readInputRegisters(START_REG, WORDS_NEEDED, buffer)) {
        return false;
    }

    data.gas1         = toFloat(buffer[0], buffer[1]);
    data.gas2         = toFloat(buffer[2], buffer[3]);
    data.gas3         = toFloat(buffer[4], buffer[5]);
    data.gas4         = toFloat(buffer[6], buffer[7]);
    data.internalTemp = toFloat(buffer[8], buffer[9]);
    data.pm25         = toFloat(buffer[10], buffer[11]);
    data.pm10         = toFloat(buffer[12], buffer[13]);
    data.externalTemp = toFloat(buffer[14], buffer[15]);
    data.humidity     = toFloat(buffer[16], buffer[17]);

    return true;
}

bool ModbusSensor::readSensorData(SensorData &data) {
    if (sensorType == SE_1) {
        return readEcoSensorData(data);
    }
    else {
        SerialMon.print("Неизвестный тип сенсора. ");
        return false;
    }
}