#include "PostureFeatures.h"
#include <math.h>

// Helper: compute pitch and roll (in degrees) from accelerometer.
// You can tweak sign conventions if needed once you see the numbers.
void computePitchRoll(const xyzFloat &acc, float &pitch_deg, float &roll_deg) {
    // pitch: rotation around Y axis (forward/backward)
    pitch_deg = atan2f(-acc.x, sqrtf(acc.y * acc.y + acc.z * acc.z)) * 180.0f / PI;
    // roll: rotation around X axis (sideways)
    roll_deg  = atan2f(acc.y, acc.z) * 180.0f / PI;
}

PostureFeatures computePostureFeatures(
    const xyzFloat &acc1,
    const xyzFloat &acc2,
    float fsr1_scaled,
    float fsr2_scaled
) {
    PostureFeatures f{};

    // --- IMU angles ---
    computePitchRoll(acc1, f.pitch1, f.roll1);
    computePitchRoll(acc2, f.pitch2, f.roll2);
    f.delta_pitch = f.pitch1 - f.pitch2;

    // --- FSRs ---
    f.fsr1_scaled = fsr1_scaled;
    f.fsr2_scaled = fsr2_scaled;
    f.fsr_total   = fsr1_scaled + fsr2_scaled;

    const float eps = 1e-3f;
    if (fabsf(f.fsr_total) > eps) {
        f.fsr_balance = (fsr1_scaled - fsr2_scaled) / f.fsr_total;
    } else {
        f.fsr_balance = 0.0f;
    }

    return f;
}

void printCsvHeader() {
    // Keep this in sync with printCsvRow
    Serial.println(
        "t_ms,"
        "pitch1,roll1,"
        "pitch2,roll2,"
        "delta_pitch,"
        "fsr1_scaled,fsr2_scaled,"
        "fsr_total,fsr_balance,"
        "label"
    );
}

void printCsvRow(uint32_t t_ms, const PostureFeatures &f, int label) {
    Serial.print(t_ms);          Serial.print(',');

    Serial.print(f.pitch1);      Serial.print(',');
    Serial.print(f.roll1);       Serial.print(',');

    Serial.print(f.pitch2);      Serial.print(',');
    Serial.print(f.roll2);       Serial.print(',');

    Serial.print(f.delta_pitch); Serial.print(',');

    Serial.print(f.fsr1_scaled); Serial.print(',');
    Serial.print(f.fsr2_scaled); Serial.print(',');

    Serial.print(f.fsr_total);   Serial.print(',');
    Serial.print(f.fsr_balance); Serial.print(',');

    Serial.println(label);
}
