#include "LogisticModel.h"
#include <math.h>

static float sigmoidf(float x) {
    // Simple sigmoid; good enough for this range
    return 1.0f / (1.0f + expf(-x));
}

float lr_predict_proba(const float features[LR_NUM_FEATURES]) {
    float z = LR_BIAS;

    for (int i = 0; i < LR_NUM_FEATURES; ++i) {
        float x_norm = (features[i] - LR_MEAN[i]) / LR_STD[i];
        z += LR_WEIGHTS[i] * x_norm;
    }

    float p_good = sigmoidf(z);
    return p_good;  // probability that posture is GOOD (label=1)
}