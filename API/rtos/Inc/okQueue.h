/**
 * @file okQueue.h
 * @author Scott Campbell
 * @brief
 * @version 0.1
 * @date 2022-01-19
 *
 * NOTE: "It is common in FreeRTOS designs for a task to receive data from more than one source. 
 * The receiving task needs to know where the data came from to determine how the data should be processed. 
 * An easy design solution is to use a single queue to transfer structures with both the value of the data 
 * and the source of the data contained in the structure’s fields." - Chapter 4.4 Mastering the FreeRTOS Kernel
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include "cmsis_os.h"
#include "common.h"
#include "logger.h"

template<typename T>
class okQueue
{
public:
    okQueue(int length) {
        handle = xQueueCreate(length, sizeof(T));
        if (handle == NULL) {
            // logger_log("there is insufficient heap RAM available for the queue to be created.");
        }
    }

    QueueHandle_t handle;
    BaseType_t status;

    int write();
    int receive(T *item, TickType_t delay = portMAX_DELAY)
    {
        status = xQueueReceive(this->handle, item, delay);
        return item;
    }
};