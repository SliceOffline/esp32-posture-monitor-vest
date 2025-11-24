#pragma once
#include <Arduino.h>
#include "model_params.h"

// features[0..LR_NUM_FEATURES-1] must be in the same order as in Python:
// pitch1_mean, pitch1_std, pitch1_min, pitch1_max, roll1_mean, ...
float lr_predict_proba(const float features[LR_NUM_FEATURES]);
