#pragma once

#include "main.h"

extern TaskHandle_t thStartCalibration;
extern TaskHandle_t thExitCalibration;
extern TaskHandle_t thCalibrate;
extern TaskHandle_t thController;

typedef enum
{
    EXIT_1VO_CALIBRATION,
    EXIT_BENDER_CALIBRATION
} CTRL_CMNDS;