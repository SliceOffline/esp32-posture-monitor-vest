#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "sensors/GyroSensor.h"
#include "sensors/PressureSensor.h"
#include "PostureFeatures.h"
#include "LogisticModel.h"
#include "model_params.h"

// --------- I2C pin & address config ---------

// Bus 0 (Wire) for IMU1
const uint8_t SDA1      = 8;
const uint8_t SCL1      = 9;
const uint8_t IMU1_ADDR = 0x68;   // AD0 = GND

// Bus 1 (Wire1) for IMU2
const uint8_t SDA2      = 11;
const uint8_t SCL2      = 12;
const uint8_t IMU2_ADDR = 0x69;   // AD0 = VCC

// --------- Pressure sensor pins/config ---------
const uint8_t PRESS_1_PIN   = 4;
const uint8_t PRESS_2_PIN   = 5;
const float   PRESS_R_FIXED = 10000.0f; // 10kΩ series resistor

// --------- Buzzer config (passive buzzer on GPIO 20) ---------
const int BUZZER_PIN      = 20;
const int BUZZER_CHANNEL  = 0;      // LEDC channel 0
const int BUZZER_FREQ_HZ  = 2000;   // 2 kHz tone
const int BUZZER_RES_BITS = 8;      // 8-bit resolution

// --------- Sampling config ---------
const uint16_t SAMPLE_PERIOD_MS = 20;   // 50 Hz

// --------- Windowing / ML config ---------
const int WINDOW_SIZE = 50;   // 1 s @ 50 Hz
const int STEP_SIZE   = 25;   // 50% overlap

// Ring buffer for last 50 samples (for runtime mode)
static PostureFeatures windowBuf[WINDOW_SIZE];
static int windowIndex = 0;           // write index
static int samplesSeen = 0;           // how many samples we've seen total
static int samplesSinceEval = 0;      // for 50% overlap

// --------- Modes ---------
enum RunMode {
    MODE_LOGGING = 0,   // stream CSV for dataset collection
    MODE_RUNTIME = 1    // real-time classification + feedback
};

// Change this to switch from data collection to live runtime
const RunMode RUN_MODE = MODE_RUNTIME;

// --------- Label for this recording session ---------
// Only for MODE_LOGGING:
// 1 = good posture session
// 0 = bad posture session
const int POSTURE_LABEL = 1;

// --------- Create sensor objects ---------

GyroSensor gyro1(&Wire,  IMU1_ADDR);
GyroSensor gyro2(&Wire1, IMU2_ADDR);

PressureSensor press1(PRESS_1_PIN, PRESS_R_FIXED, 3.3f, 1.0f);
PressureSensor press2(PRESS_2_PIN, PRESS_R_FIXED, 3.3f, 1.0f);

// Init status
bool g1_ok = false;
bool g2_ok = false;
bool p1_ok = false;
bool p2_ok = false;

// --------- Buzzer helper functions ---------

void buzzerInit() {
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ_HZ, BUZZER_RES_BITS);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    // make sure it's off
    ledcWrite(BUZZER_CHANNEL, 0);
}

void buzzerBeepOnce(int duration_ms = 100) {
    // Turn on tone (duty > 0)
    ledcWrite(BUZZER_CHANNEL, 128);  // 50% duty
    delay(duration_ms);
    // Turn off
    ledcWrite(BUZZER_CHANNEL, 0);
}

void buzzerDoubleBeep() {
    buzzerBeepOnce(100);
    delay(100);
    buzzerBeepOnce(100);
}

// --------- BAD posture alert logic ---------
static bool lastWasBad = false;
static uint32_t badStartTime = 0;     // when bad posture began
static const uint32_t BAD_DURATION_MS = 3000;   // 3 seconds

// --------- Window feature computation ---------

// Map PostureFeatures -> 36 window features in the same order as Python:
// [0] pitch1_mean,  [1] pitch1_std,  [2] pitch1_min,  [3] pitch1_max,
// [4] roll1_mean,   [5] roll1_std,   [6] roll1_min,   [7] roll1_max,
// [8] pitch2_mean,  [9] pitch2_std,  [10] pitch2_min, [11] pitch2_max,
// [12] roll2_mean,  [13] roll2_std,  [14] roll2_min,  [15] roll2_max,
// [16] delta_pitch_mean, [17] delta_pitch_std, [18] delta_pitch_min, [19] delta_pitch_max,
// [20] fsr1_scaled_mean, [21] fsr1_scaled_std, [22] fsr1_scaled_min, [23] fsr1_scaled_max,
// [24] fsr2_scaled_mean, [25] fsr2_scaled_std, [26] fsr2_scaled_min, [27] fsr2_scaled_max,
// [28] fsr_total_mean,   [29] fsr_total_std,   [30] fsr_total_min,   [31] fsr_total_max,
// [32] fsr_balance_mean, [33] fsr_balance_std, [34] fsr_balance_min, [35] fsr_balance_max.
static void computeWindowFeatures(const PostureFeatures *buf,
                                  int n,
                                  float outFeatures[LR_NUM_FEATURES]) {
    // 9 base signals
    const int S = 9;
    float sum[S]   = {0};
    float sumSq[S] = {0};
    float vmin[S];
    float vmax[S];

    // Initialize mins/maxes with first sample
    vmin[0] = vmax[0] = buf[0].pitch1;
    vmin[1] = vmax[1] = buf[0].roll1;
    vmin[2] = vmax[2] = buf[0].pitch2;
    vmin[3] = vmax[3] = buf[0].roll2;
    vmin[4] = vmax[4] = buf[0].delta_pitch;
    vmin[5] = vmax[5] = buf[0].fsr1_scaled;
    vmin[6] = vmax[6] = buf[0].fsr2_scaled;
    vmin[7] = vmax[7] = buf[0].fsr_total;
    vmin[8] = vmax[8] = buf[0].fsr_balance;

    for (int i = 0; i < n; ++i) {
        float vals[S] = {
            buf[i].pitch1,
            buf[i].roll1,
            buf[i].pitch2,
            buf[i].roll2,
            buf[i].delta_pitch,
            buf[i].fsr1_scaled,
            buf[i].fsr2_scaled,
            buf[i].fsr_total,
            buf[i].fsr_balance
        };

        for (int k = 0; k < S; ++k) {
            float x = vals[k];
            sum[k]   += x;
            sumSq[k] += x * x;
            if (x < vmin[k]) vmin[k] = x;
            if (x > vmax[k]) vmax[k] = x;
        }
    }

    // Compute mean/std/min/max in the correct order
    int idx = 0;
    for (int k = 0; k < S; ++k) {
        float mean = sum[k] / n;
        float var  = (sumSq[k] / n) - (mean * mean);
        if (var < 0.0f) var = 0.0f;   // numerical safety
        float std  = sqrtf(var);

        outFeatures[idx++] = mean;
        outFeatures[idx++] = std;
        outFeatures[idx++] = vmin[k];
        outFeatures[idx++] = vmax[k];
    }
    // idx should be 36 when done.
}

// ---------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);   // allow Serial to come up

    // --- Buzzer setup ---
    buzzerInit();

    // --- I2C buses setup ---
    Wire.begin(SDA1, SCL1);
    Wire.setClock(100000);  // 100 kHz

    Wire1.begin(SDA2, SCL2);
    Wire1.setClock(100000); // 100 kHz

    // --- Gyro init ---
    g1_ok = gyro1.begin();
    g2_ok = gyro2.begin();

    // --------- 3s pre-calibration delay ---------
    // This gives you time to sit in your "neutral good posture"
    // after plugging in the USB.
    Serial.println("Please sit still in neutral posture. Calibrating in 3 seconds...");
    delay(3000);

    // --- Calibrate IMUs (while you're sitting straight) ---
    if (g1_ok) {
        gyro1.calibrate();
    }
    if (g2_ok) {
        gyro2.calibrate();
    }

    // --- Calibration done: double beep ---
    buzzerDoubleBeep();
    Serial.println("Calibration complete.");

    // --- Pressure sensors init ---
    p1_ok = press1.begin();
    p2_ok = press2.begin();

    // --- CSV header only in logging mode ---
    if (RUN_MODE == MODE_LOGGING) {
        printCsvHeader();
    }
}

void loop() {
    static uint32_t lastSampleMs = 0;
    uint32_t now = millis();

    if (now - lastSampleMs < SAMPLE_PERIOD_MS) {
        return;
    }
    lastSampleMs += SAMPLE_PERIOD_MS;  // maintain ~50 Hz

    // --------- Read raw sensors ---------
    xyzFloat acc1{0.0f, 0.0f, 0.0f};
    xyzFloat acc2{0.0f, 0.0f, 0.0f};

    if (g1_ok && gyro1.isOk()) {
        acc1 = gyro1.readAccel();  // in g
    }
    if (g2_ok && gyro2.isOk()) {
        acc2 = gyro2.readAccel();
    }

    float fsr1_scaled = 0.0f;
    float fsr2_scaled = 0.0f;

    if (p1_ok && press1.isOk()) {
        fsr1_scaled = press1.readScaled();
    }
    if (p2_ok && press2.isOk()) {
        fsr2_scaled = press2.readScaled();
    }

    // --------- Compute features (shared by both modes) ---------
    PostureFeatures feats = computePostureFeatures(
        acc1,
        acc2,
        fsr1_scaled,
        fsr2_scaled
    );

    // Update ring buffer (for runtime mode)
    windowBuf[windowIndex] = feats;
    windowIndex = (windowIndex + 1) % WINDOW_SIZE;
    if (samplesSeen < WINDOW_SIZE) {
    samplesSeen++;
    }
    samplesSinceEval++;

    // --------- Mode-specific behavior ---------
    if (RUN_MODE == MODE_LOGGING) {
        // Data collection: output CSV row per sample
        printCsvRow(now, feats, POSTURE_LABEL);

    } else if (RUN_MODE == MODE_RUNTIME) {
        // --- RUNTIME MODE: classify posture every STEP_SIZE samples ---
        if (samplesSeen >= WINDOW_SIZE && samplesSinceEval >= STEP_SIZE) {
            samplesSinceEval = 0;

            // Copy the WINDOW_SIZE most recent samples into a linear array
            PostureFeatures window[WINDOW_SIZE];
            // windowIndex points to next write position, so the oldest sample
            // is at windowIndex, and the newest is at windowIndex - 1.
            for (int i = 0; i < WINDOW_SIZE; ++i) {
            int srcIndex = (windowIndex + i) % WINDOW_SIZE;
            window[i] = windowBuf[srcIndex];
            }

            float feats36[LR_NUM_FEATURES];
            computeWindowFeatures(window, WINDOW_SIZE, feats36);

            float p_good = lr_predict_proba(feats36);
            bool isBad = (p_good < 0.5f);

            // Debug print
            Serial.print("p_good = ");
            Serial.print(p_good, 3);
            Serial.print("  -> ");
            Serial.println(isBad ? "BAD" : "GOOD");

            // --- BAD posture timer logic ---
            uint32_t nowMs = millis();

            if (isBad) {
                if (!lastWasBad) {
                     // Just switched into BAD posture → start timer
                    badStartTime = nowMs;
                    lastWasBad = true;
                } else {
                    // Already bad → check if 3 seconds passed
                    if (nowMs - badStartTime >= BAD_DURATION_MS) {
                        // Beep alert
                        buzzerBeepOnce(200);   // adjust duration as you like

                        // Reset timer for next beep
                         badStartTime = nowMs;
                    }
                }

            } else {
                // Posture is GOOD → reset state
                lastWasBad = false;
                badStartTime = nowMs;
            }

        }

    }
}
