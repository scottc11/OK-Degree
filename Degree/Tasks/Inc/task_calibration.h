#pragma once

#include "main.h"
#include "Algorithms.h"
#include "TouchChannel.h"
#include "GlobalControl.h"
#include "MultiChanADC.h"
#include "logger.h"
#include "ArrayMethods.h"
#include "PitchFrequencies.h"

#define CALIBRATION_DAC_SETTLE_TIME 2       // how long in ticks the task should delay to let the DAC settle (before sampling the frequency again)
#define CALIBRATION_SAMPLE_RATE_HZ  16000   // set ADC timer overflow frequency to 16000hz (twice the freq of B8)
#define DEFAULT_VOLTAGE_ADJMNT 400
#define MAX_CALIB_ATTEMPTS 9               // how many times the calibrator will try and match the given frequency
#define MAX_FREQ_SAMPLES 25                // how many frequency calculations we want to use to obtain our average frequency prediction of the input. The higher the number, the more accurate the result
#define ZERO_CROSS_THRESHOLD 2000          // for handling hysterisis at zero crossing point NOTE: If set around 500 freq readings become very unstable

void initializeCalibration();
void taskObtainSignalFrequency(void *params);
float calculateAverageFreq();
void taskCalibrate(void *params);