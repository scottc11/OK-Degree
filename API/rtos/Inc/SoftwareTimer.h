#pragma once

#include "cmsis_os.h"
#include "Callback.h"

class SoftwareTimer {
public:
    SoftwareTimer(){};

    TimerHandle_t handle;

    Callback<void> callback;

    // If the timer had already been started and was already in the active state, then xTimerStart() has equivalent functionality to the xTimerReset() API function.
    void start()
    {
        if (xTimerIsTimerActive(handle))
        {
            xTimerStart(handle, 0);
        }
    }

    void stop();

    void reset();
};