#pragma once

#include <Arduino.h>

class PressureSensor {
public:
    // analogPin   = GPIO used for ADC
    // rFixedOhms  = fixed series resistor value (10000.0f)
    // vRef        = reference voltage (3.3f)
    // pressedThreshold = threshold on "scaled" value for isPressed
    PressureSensor(uint8_t analogPin,
                   float rFixedOhms,
                   float vRef = 3.3f,
                   float pressedThreshold = 1.0f);

    // Initialize sensor
    bool begin();

    int   readRaw();            // ADC raw (0..4095)
    float readVoltage();        // node voltage in Volts
    float readResistance();     // sensor resistance in Ohms (INF if open)
    float readScaled();         // scaled value - “pressure”
    bool  isPressed();          // simple pressed / not pressed

    bool  isOk() const { return _ok; }  // always true because we trust wiring

private:
    uint8_t _pin;
    float   _rFixed;
    float   _vRef;
    float   _pressedThreshold;
    int     _adcMax;
    bool    _ok;
};
