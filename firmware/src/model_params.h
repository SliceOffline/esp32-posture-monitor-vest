#pragma once
#include <Arduino.h>

constexpr int LR_NUM_FEATURES = 36;

extern const float LR_WEIGHTS[LR_NUM_FEATURES];
extern const float LR_MEAN[LR_NUM_FEATURES];
extern const float LR_STD[LR_NUM_FEATURES];
extern const float LR_BIAS;
