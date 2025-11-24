#include "GyroSensor.h"

GyroSensor::GyroSensor(TwoWire* bus, uint8_t addr)
: _bus(bus),
  _imu(bus, addr),
  _address(addr),
  _initialized(false)
{}

bool GyroSensor::begin() {
    // Try init
    if (!_imu.init()) {
        Serial.print("GyroSensor at 0x");
        Serial.print(_address, HEX);
        Serial.println(" init FAILED");
        _initialized = false;
        return false;
    }

    // Configure ranges
    _imu.setGyrRange(MPU6500_GYRO_RANGE_250);
    _imu.setAccRange(MPU6500_ACC_RANGE_2G);

    _initialized = true;

    Serial.print("GyroSensor at 0x");
    Serial.print(_address, HEX);
    Serial.println(" init OK");
    return true;
}

void GyroSensor::calibrate() {
    if (!_initialized) return;
    _imu.autoOffsets();
}

xyzFloat GyroSensor::readAccel() {
    if (!_initialized) {
        return xyzFloat{0.0f, 0.0f, 0.0f};
    }
    return _imu.getGValues();
}

xyzFloat GyroSensor::readGyro() {
    if (!_initialized) {
        return xyzFloat{0.0f, 0.0f, 0.0f};
    }
    return _imu.getGyrValues();
}
