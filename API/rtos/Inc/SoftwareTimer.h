#pragma once

#include "cmsis_os.h"
#include "Callback.h"

class SoftwareTimer {
public:
    SoftwareTimer(void (*func)(), TickType_t period, bool repeated);
    ~SoftwareTimer();

    void start(TickType_t delay = 0);
    void stop();
    void reset(TickType_t delay = 0);
    TickType_t period();

private:
    TimerHandle_t handle;
    void (*callback)();

    static void callbackHandler(TimerHandle_t xTimer);
};