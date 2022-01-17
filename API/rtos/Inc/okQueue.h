#pragma once

#include "cmsis_os.h"

class okQueue
{
    okQueue() {}

    QueueHandle_t handle;

    int write();
    int read();
};