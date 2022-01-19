/**
 * @file OK_Semaphore.h
 * @author Scott Campbell
 * @brief
 * @version 0.1
 * @date 2022-01-11
 *
 * @copyright Copyright (c) 2022
 *
 * I think the idea of a semphore is to notifiy one task to execute code using a "shared resource" after another task finishes executing its code that uses the "shared resource".
 * Ex. Mutliple tasks using the same DAC via the same SPI peripheral
 * 
 * It differs from a queue because there is no data being passed between tasks, more so just a notification / flag
 *
 */

#pragma once

#include "cmsis_os.h"

class okSemaphore {
public:
    okSemaphore(){
        handle = xSemaphoreCreateBinary();
    };

    ~okSemaphore() {
        vSemaphoreDelete(handle);
    }

    SemaphoreHandle_t handle;

    void give() {
        xSemaphoreGive(handle);
    }

    void giveISR() {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    void take(TickType_t delay = portMAX_DELAY)
    {
        xSemaphoreTake(handle, delay);
    }

    void takeISR();
};