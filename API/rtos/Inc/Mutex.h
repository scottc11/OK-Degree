/**
 * @file okMutex.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-02-01
 *
 * @copyright Copyright (c) 2022
 *
 *
 * In a multitasking system there is potential for error if one task starts to access a resource,
 * but does not complete its access before being transitioned out of the Running state. If the
 * task leaves the resource in an inconsistent state, then access to the same resource by any other
 * task or interrupt could result in data corruption, or other similar issue.
 *
 * NOTE: To ensure data consistency is maintained at all times access to a resource that is shared between
 * tasks, or between tasks and interrupts, must be managed using a ‘mutual exclusion’ technique.
 *
 * The goal is to ensure that, once a task starts to access a shared resource that is not re-entrant and
 * not thread-safe, the same task has exclusive access to the resource until the resource has been
 * returned to a consistent state.
 *
 * NOTE: A switch to another task cannot occur between the call to taskENTER_CRITICAL() and the call 
 * to taskEXIT_CRITICAL(). Interrupts may still execute on FreeRTOS ports that allow interrupt nesting, 
 * but only interrupts whose logical priority is above the value assigned to the 
 * configMAX_SYSCALL_INTERRUPT_PRIORITY constant – and those interrupts are not permitted to call 
 * FreeRTOS API functions.
 *
 *
 * NOTE: Non-atomic Access to Variables
 *
 * Updating multiple members of a structure, or updating a variable that is larger than the natural 
 * word size of the architecture (for example, updating a 32-bit variable on a 16-bit machine), are 
 * examples of non-atomic operations. If they are interrupted, they can result in data loss or corruption.
 *
 * NOTE: Function Reentrancy
 *
 * A function is ‘reentrant’ if it is safe to call the function from more than one task, or from
 * both tasks and interrupts. Reentrant functions are said to be ‘thread safe’ because they can
 * be accessed from more than one thread of execution without the risk of data or logical operations
 * becoming corrupted.
 *
 * Each task maintains its own stack and its own set of processor (hardware) register values. If a
 * function does not access any data other than data stored on the stack or held in a register, then
 * the function is reentrant, and thread safe. Listing 112 is an example of a reentrant function.
 * Listing 113 is an example of a function that is not reentrant.
 */

#pragma once


#include "cmsis_os.h"

class Mutex {
public:
    Mutex();

    void lock(TickType_t wait = portMAX_DELAY);
    void unlock();

private:
    SemaphoreHandle_t _handle;
};