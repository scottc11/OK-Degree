#pragma once

#include "main.h"

extern TaskHandle_t thStartCalibration;
extern TaskHandle_t thExitCalibration;
extern TaskHandle_t thCalibrate;
extern TaskHandle_t thController;
extern TaskHandle_t tuner_task_handle;

extern QueueHandle_t tuner_queue;

typedef enum
{
    EXIT_1VO_CALIBRATION,
    EXIT_BENDER_CALIBRATION,
    ENTER_VCO_TUNING,
} CTRL_CMNDS;