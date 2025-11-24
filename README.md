# ESP32 Posture Monitoring Vest

A wearable posture monitoring system based on an ESP32-S3 microcontroller, using
two MPU6500 IMUs, two FSR pressure sensors and an on-device Logistic Regression
classifier. Developed as part of an undergraduate thesis project.

## Overview

The system detects good vs bad sitting posture in real time.  
It includes:
- **Firmware (C++ / PlatformIO)** for sensor sampling, feature extraction,
  windowing, normalization and on-device classification.
  Also controls buzzer mechanism for user alerts.
- **Python scripts** for data logging, preprocessing, dataset creation and
  machine-learning model training.

## Repository Structure

- firmware/ – ESP32-S3 PlatformIO project
- python/ – Python tools for logging & ML
- data/ – session recordings in CSV format


## Requirements

**Firmware:** PlatformIO, ESP32-S3 board  
**Python:** Python 3.10+, pandas, numpy, scikit-learn, pyserial

## Usage

### Data Logging
1. Flash firmware in MODE_LOGGING.  
2. Run: python python/log_serial.py  The script records a 2-minute CSV session via serial.

### Model Training

1. Build windowed dataset:
python python/build_windows_dataset.py

2. Evaluate models (LogReg, SVM, Random Forest, MLP):

python python/compare_models_random_split.py
python python/compare_models_sessionwise.py

3. Export Logistic Regression parameters for deployment:

python python/export_logreg_params.py

4. Copy results into firmware/src/model_params.cpp

### Runtime Mode

Flash firmware in MODE_RUNTIME to enable real-time posture classification and
buzzer alerts.


