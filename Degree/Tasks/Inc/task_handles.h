#pragma once

#include "main.h"
#include "BitwiseMethods.h"

#define TUNING_TOLERANCE 0.1f // tolerable frequency tuning difference

extern TaskHandle_t thStartCalibration;
extern TaskHandle_t thExitCalibration;
extern TaskHandle_t thCalibrate;
extern TaskHandle_t thController;
extern TaskHandle_t tuner_task_handle;

extern QueueHandle_t tuner_queue;

enum CTRL_CMNDS
{
    ENTER_1VO_CALIBRATION = 0,
    EXIT_1VO_CALIBRATION = 1,
    EXIT_BENDER_CALIBRATION = 2,
    ENTER_VCO_TUNING = 3,
    EXIT_VCO_TUNING = 4
};
typedef enum CTRL_CMNDS CTRL_CMNDS;

uint32_t create_controller_command(uint16_t channel, uint16_t command);
CTRL_CMNDS noti_get_command(uint32_t notification);
uint8_t noti_get_channel(uint32_t notification);
void ctrl_send_command(uint8_t channel, CTRL_CMNDS command);