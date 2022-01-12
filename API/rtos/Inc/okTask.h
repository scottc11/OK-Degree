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

class okTask
{
public:
    okTask(char *_name, void (*_func)(void *))
    {
        name = _name;
        task_func = _func;
        xTaskCreate(this->startTask, name, 100, this, 1, &this->handle);
    };

    char *name;
    TaskHandle_t handle;
    void (*task_func)(void *);

    static void startTask(void *_this)
    {
        static_cast<okTask *>(_this)->task_func(NULL);
    }
};