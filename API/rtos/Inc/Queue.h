/**
 * @file Queue.h
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
#include "logger.h"

template<class T, unsigned L>
class Queue
{
public:
    Queue() {
        handle = xQueueCreate(L, sizeof(T));
        if (handle == NULL) {
            // logger_log("there is insufficient heap RAM available for the queue to be created.");
        }
    }

    QueueHandle_t handle;
    BaseType_t status;

    /**
     * @brief When an item is added to a queue with available space, the addition happens immediately. Because 
     * space was available in the queue, the task sending the item to the queue continues running, unless 
     * there is another higher priority task waiting on an item to appear in the queue.
     *
     * NOTE: When a queue is full, no information is thrown away. Instead, the task attempting to send the item 
     * to the queue will wait for up to a predetermined amount of time for available space in the queue.
     * 
     * @param item The item to put on the Queue. NOTE: currently being copied instead of referenced
     * @param delay How long to wait for room if Queue is full.
     * @return int
     */
    BaseType_t send(T item, TickType_t delay = portMAX_DELAY)
    {
        return xQueueSend(this->handle, (void *) item, delay);
    }

    /**
     * @brief Get an item from the Queue.
     * Gets the first item from the Queue
     * @param buffer_ptr Variable to place the item
     * @param delay How long to wait for an item to be available.
     * @return True if an item returned.
     */
    BaseType_t receive(T *buffer_ptr, TickType_t delay = portMAX_DELAY)
    {
        return xQueueReceive(this->handle, buffer_ptr, delay);
    }
};