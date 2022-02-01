/**
 * @file okTask.h
 * @author your name (you@domain.com)
 * @brief based on answer from https://stackoverflow.com/questions/45831114/c-freertos-task-invalid-use-of-non-static-member-function
 * @version 0.1
 * @date 2022-01-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include "cmsis_os.h"
#include "logger.h"

class okTask
{
public:
    okTask(char *_name, void (*_func)(void *))
    {
        name = _name;
        task_func = _func;
        BaseType_t status;
        status = xTaskCreate(this->startTask, name, 100, this, 1, &this->handle);
        if (status != pdPASS) {
            logger_log("Task Create COULD_NOT_ALLOCATE_REQUIRED_MEMORY\n");
            logger_log("TASK: ");
            logger_log(name);
            logger_log("\n");
        }
    };

    char *name;
    TaskHandle_t handle;
    void (*task_func)(void *);

    static void startTask(void *_this)
    {
        static_cast<okTask *>(_this)->task_func(NULL);
    }
};