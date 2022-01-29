#pragma once

#include "main.h"
#include "TouchChannel.h"
#include "MultiChanADC.h"
#include "logger.h"
#include "ArrayMethods.h"
#include "PitchFrequencies.h"

#define DEFAULT_VOLTAGE_ADJMNT 200
#define MAX_CALIB_ATTEMPTS 20         // how many times the calibrator will try and match the given frequency
#define MAX_FREQ_SAMPLES 25           // how many frequency calculations we want to use to obtain our average frequency prediction of the input. The higher the number, the more accurate the result
#define ZERO_CROSS_THRESHOLD 1000     // for handling hysterisis at zero crossing point NOTE: If set around 500 freq readings become very unstable
#define TUNING_TOLERANCE     0.1f     // tolerable frequency tuning difference

void initializeCalibration();
void taskObtainSignalFrequency(void *params);
float calculateAverageFreq();
void taskCalibrate(void *params);