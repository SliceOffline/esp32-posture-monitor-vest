#include "PressureSensor.h"
#include <math.h>   // for INFINITY, isfinite

PressureSensor::PressureSensor(uint8_t analogPin,
                               float rFixedOhms,
                               float vRef,
                               float pressedThreshold)
: _pin(analogPin),
  _rFixed(rFixedOhms),
  _vRef(vRef),
  _pressedThreshold(pressedThreshold),
  _adcMax(4095),   // ESP32 default 12-bit ADC
  _ok(false)
{}

bool PressureSensor::begin() {
    pinMode(_pin, INPUT);
    // analogReadResolution(12); // just in case

    _ok = true;  // trust the wiring

    Serial.print("PressureSensor on pin ");
    Serial.print(_pin);
    Serial.println(" initialized.");
    return true;
}

int PressureSensor::readRaw() {
    return analogRead(_pin);
}

float PressureSensor::readVoltage() {
    int raw = readRaw();
    return (static_cast<float>(raw) / _adcMax) * _vRef;
}

float PressureSensor::readResistance() {
    int raw = readRaw();

    float vNode = (static_cast<float>(raw) / _adcMax) * _vRef;

    // Open circuit / weird reading
    if (vNode <= 0.0001f) {
        return INFINITY;
    }

    // My wiring:
    // Vnode = Vref * (Rfixed / (Rfixed + Rfsr))
    // => Rfsr = Rfixed * (Vref - Vnode) / Vnode
    float numerator   = _rFixed * (_vRef - vNode);
    float denominator = vNode;

    if (denominator <= 0.0f) {
        return INFINITY;
    }

    return numerator / denominator;
}

float PressureSensor::readScaled() {
    float r = readResistance();

    if (!isfinite(r) || r <= 0.0f) {
        return 0.0f;
    }

    // Smaller R = more pressure
    const float k = 10000.0f; // scaling constant
    return k / r;
}

bool PressureSensor::isPressed() {
    float s = readScaled();
    return s >= _pressedThreshold;
}
