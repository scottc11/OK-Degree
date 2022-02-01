#include "task_controller.h"

void task_controller(void *params)
{
    GlobalControl *controller = (GlobalControl *)params;

    thController = xTaskGetCurrentTaskHandle();

    while (1)
    {
        // you could bit mask the first 16 bits to determine the command, and the bottom 16 bits to determine the channel to act on.
        uint32_t notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint8_t channel = noti_get_channel(notification);
        uint16_t command = noti_get_command(notification);

        switch (command)
        {
        case CTRL_CMNDS::ENTER_1VO_CALIBRATION:
            ctrl_send_command(channel, CTRL_CMNDS::ENTER_VCO_TUNING);
            break;
        case CTRL_CMNDS::EXIT_1VO_CALIBRATION:
            vTaskDelete(thCalibrate);
            vTaskDelete(thStartCalibration);
            // offload all this shit to a task with a much higher stack size
            multi_chan_adc_set_sample_rate(&hadc1, &htim3, ADC_SAMPLE_RATE_HZ);

            controller->saveCalibrationDataToFlash();

            controller->disableVCOCalibration();
            break;
        case CTRL_CMNDS::EXIT_BENDER_CALIBRATION:
            // do something
            break;
        case CTRL_CMNDS::ENTER_VCO_TUNING:
            xTaskCreate(taskObtainSignalFrequency, "detector", RTOS_STACK_SIZE_MIN, controller->channels[channel], RTOS_PRIORITY_MED, &thStartCalibration);
            xTaskCreate(task_tuner, "tuner", RTOS_STACK_SIZE_MIN, controller->channels[channel], RTOS_PRIORITY_HIGH, &tuner_task_handle);
            break;
        case CTRL_CMNDS::EXIT_VCO_TUNING:
            vTaskDelete(tuner_task_handle);
            controller->display->fill(controller->selectedChannel);
            controller->display->flash(3, 200);
            controller->display->clear(controller->selectedChannel);
            controller->mode = GlobalControl::CALIBRATING_1VO;
            xTaskCreate(taskCalibrate, "calibrate", RTOS_STACK_SIZE_MIN, controller->channels[controller->selectedChannel], RTOS_PRIORITY_MED, &thCalibrate);
            break;
        default:
            break;
        }
    }
}