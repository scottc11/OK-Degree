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
        CTRL_ACTION action = noti_get_command(notification);

        switch (action)
        {
        case CTRL_ACTION::ENTER_1VO_CALIBRATION:
            vTaskSuspend(main_task_handle);
            suspend_sequencer_task();
            controller->channels[channel]->initializeCalibration();
            ctrl_dispatch(CTRL_ACTION::ENTER_VCO_TUNING, channel, 0);
            break;
        case CTRL_ACTION::EXIT_1VO_CALIBRATION:
            vTaskDelete(thCalibrate);
            vTaskDelete(thStartCalibration);
            for (int i = 0; i < 4; i++)
            {
                controller->channels[i]->adc.queueSample = false;
            }
            
            // offload all this shit to a task with a much higher stack size
            multi_chan_adc_set_sample_rate(&hadc1, &htim3, ADC_SAMPLE_RATE_HZ);

            controller->saveCalibrationDataToFlash();

            controller->disableVCOCalibration();
            vTaskResume(main_task_handle);
            resume_sequencer_task();
            break;
            
        case CTRL_ACTION::EXIT_BENDER_CALIBRATION:
            // do something
            break;

        case CTRL_ACTION::ENTER_VCO_TUNING:
            xTaskCreate(task_tuner, "tuner", RTOS_STACK_SIZE_MIN, controller->channels[channel], RTOS_PRIORITY_HIGH, &tuner_task_handle);
            xTaskCreate(taskObtainSignalFrequency, "detector", RTOS_STACK_SIZE_MIN, controller->channels[channel], RTOS_PRIORITY_MED, &thStartCalibration);
            break;

        case CTRL_ACTION::EXIT_VCO_TUNING:
            vTaskDelete(tuner_task_handle);
            controller->display->setColumn(7, PWM::PWM_HIGH, false);
            controller->display->setColumn(8, PWM::PWM_HIGH, false);
            controller->display->flash(3, 200);
            controller->display->clear();
            controller->display->fill(30, true);
            controller->mode = GlobalControl::VCO_CALIBRATION;
            xTaskCreate(taskCalibrate, "calibrate", RTOS_STACK_SIZE_MIN, controller->channels[controller->selectedChannel], RTOS_PRIORITY_MED, &thCalibrate);
            break;

        case CTRL_ACTION::ADC_SAMPLING_PROGRESS:
            // channel -> which channel
            // data    -> value between 0..100
            // uint8_t channel = (uint8_t)bitwise_slice(notification, 8, 8);
            // uint16_t progress = (uint16_t)bitwise_slice(notification, 16, 16);
            break;

        case CTRL_ACTION::CONFIG_SAVE:
            suspend_sequencer_task();
            break;
        default:
            break;
        }
        logger_log_task_watermark(); // TODO: delete for production
    }
}