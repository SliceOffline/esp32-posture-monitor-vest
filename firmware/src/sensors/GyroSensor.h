#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <MPU6500_WE.h>
#include "xyzFloat.h"

class GyroSensor {
public:
    // bus  = &Wire or &Wire1
    // addr = 0x68 or 0x69 etc.
    GyroSensor(TwoWire* bus, uint8_t addr);

    // Call after Wire/Wire1.begin(...)
    bool begin();

    // Optional: run auto offset calibration
    void calibrate();

    // Read accelerometer in g
    xyzFloat readAccel();

    // Read gyroscope in deg/s
    xyzFloat readGyro();

    bool isOk() const { return _initialized; }

private:
    TwoWire*    _bus;
    MPU6500_WE  _imu;
    uint8_t     _address;
    bool        _initialized;
};
