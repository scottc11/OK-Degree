#pragma once

#include "main.h"
#include "MultiChanADC.h"
#include "logger.h"
#include "GlobalControl.h"

using namespace DEGREE;

void task_controller(void *params) {
    GlobalControl *controller = (GlobalControl *)params;

    thController = xTaskGetCurrentTaskHandle();

    while (1)
    {
        uint32_t command = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        switch (command)
        {
        case EXIT_1VO_CALIBRATION:
            logger_log_task_watermark();
            // flash the grid of leds on and off for a sec then exit
            controller->display->flash(3, 300);
            controller->display->clear();
            logger_log_task_watermark();

            vTaskDelete(thCalibrate);
            vTaskDelete(thStartCalibration);
            // offload all this shit to a task with a much higher stack size
            multi_chan_adc_set_sample_rate(&hadc1, &htim3, ADC_SAMPLE_RATE_HZ);

            controller->saveCalibrationDataToFlash();

            controller->disableVCOCalibration();
            break;
        case EXIT_BENDER_CALIBRATION:
            // do something
            break;
        default:
            break;
        }
    }
    
}