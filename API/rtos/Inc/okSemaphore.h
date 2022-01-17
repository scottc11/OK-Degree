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

SemaphoreHandle_t xBinaraySemaphore;

void main() {
    xBinaraySemaphore = xSemaphoreCreateBinary();
    
    // in order to 'get the ball rolling', you need to first 'give' the semaphore ie. release access to it ie. let it be taken.
    xSemaphoreGive(xBinaraySemaphore);
}

void task(void *params) {
    while (1)
    {
        xSemaphoreTake(xBinaraySemaphore, portMAX_DELAY);
        // do something
        xSemaphoreGive(xBinaraySemaphore);
    }
}