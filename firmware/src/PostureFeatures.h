#pragma once

#include <Arduino.h>
#include "xyzFloat.h"

// All per-sample features we care about for logging / runtime
struct PostureFeatures {
    // Derived angles from accelerometer
    float pitch1;       // upper IMU pitch (deg)
    float roll1;        // upper IMU roll  (deg)
    float pitch2;       // lower IMU pitch (deg)
    float roll2;        // lower IMU roll  (deg)
    float delta_pitch;  // pitch1 - pitch2 (spinal curvature indicator)

    // FSR-derived
    float fsr1_scaled;
    float fsr2_scaled;
    float fsr_total;
    float fsr_balance;
};

// Compute pitch/roll from accelerometer vector (in g)
// Note: sign conventions may need tweaking depending on physical IMU orientation.
void computePitchRoll(const xyzFloat &acc, float &pitch_deg, float &roll_deg);

// Build a PostureFeatures struct from raw sensor values
PostureFeatures computePostureFeatures(
    const xyzFloat &acc1,
    const xyzFloat &acc2,
    float fsr1_scaled,
    float fsr2_scaled
);

// Print CSV header (column names)
void printCsvHeader();

// Print one CSV row: timestamp, all features, label
void printCsvRow(uint32_t t_ms, const PostureFeatures &f, int label);
