#pragma once

#include "common.h"

char MEM_POOL_STRING_1[50];
char MEM_POOL_STRING_2[50];

void task_logger(void *params)
{
    
    logger_queue = xQueueCreate(32, sizeof(uint16_t));
    while (1)
    {
        if (xQueueReceive(logger_queue, &pulse, portMAX_DELAY))
        {
            logger_log("\n");
            logger_log(pulse);
        }
    }
}

/**
4.5 Working with Large or Variable Sized Data

Queuing Pointers

If the size of the data being stored in the queue is large, then it is preferable to use the 
queue to transfer pointers to the data, rather than copy the data itself into and out of 
the queue byte by byte. Transferring pointers is more efficient in both processing time and 
the amount of RAM required to create the queue. However, when queuing pointers, extreme care 
must be taken to ensure that:

1. The owner of the RAM being pointed to is clearly defined.

    When sharing memory between tasks via a pointer, it is essential to ensure that both tasks 
    do not modify the memory contents simultaneously, or take any other action that could cause 
    the memory contents to be invalid or inconsistent. Ideally, only the sending task should be 
    permitted to access the memory until a pointer to the memory has been queued, and only the 
    receiving task should be permitted to access the memory after the pointer has been received 
    from the queue.

2. The RAM being pointed to remains valid.
    If the memory being pointed to was allocated dynamically, or obtained from a pool of pre-allocated 
    buffers, then exactly one task should be responsible for freeing the memory. No tasks should 
    attempt to access the memory after it has been freed.
    
    A pointer should never be used to access data that has been allocated on a task stack. The data will not be valid after the stack frame has changed.

*/