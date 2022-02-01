#include "task_handles.h"

TaskHandle_t thStartCalibration;
TaskHandle_t thExitCalibration;
TaskHandle_t thCalibrate;
TaskHandle_t thController;

uint32_t create_controller_command(uint16_t channel, uint16_t command)
{
    return two16sTo32(channel, command);
}

CTRL_CMNDS noti_get_command(uint32_t notification)
{
    return (CTRL_CMNDS)bitwise_last_16_of_32(notification);
}

uint8_t noti_get_channel(uint32_t notification)
{
    return (uint8_t)bitwise_first_16_of_32(notification);
}

void ctrl_send_command(uint8_t channel, CTRL_CMNDS command)
{
    uint32_t notification = create_controller_command(channel, command);
    xTaskNotify(thController, notification, eSetValueWithOverwrite);
}