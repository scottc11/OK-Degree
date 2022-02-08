#pragma once

#include "cmsis_os.h"

class Notification {
public:
    Notification(TaskHandle_t *_taskHandle) {
        taskHandle = _taskHandle;
    };

    TaskHandle_t *taskHandle;

    void send(uint32_t value, eNotifyAction action)
    {
        xTaskNotify(*taskHandle, value, action);
    }

    void sendISR(uint32_t value, eNotifyAction action) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xTaskNotifyFromISR(*taskHandle, value, action, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    uint32_t recieve(TickType_t time_to_wait = portMAX_DELAY) {
        return ulTaskNotifyTake(pdTRUE, time_to_wait);
    }
};