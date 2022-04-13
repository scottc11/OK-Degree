/**
 * @file SoftwareTimer.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-04-13
 *
 * @copyright Copyright (c) 2022
 *
 * Example Initialization
    Callback<void()> cb = callback(some_static_function);
    SoftwareTimer timer(3000, false);
    timer.attachCallback(cb);
 *
 */

#pragma once

#include "cmsis_os.h"
#include "Callback.h"

/**
 * @brief Software timers are used to schedule the execution of a function at a set time in the future,
 * or periodically with a fixed frequency. The function executed by the software timer is called the
 * software timer’s callback function.
 *
 * @note Software timers do not use any processing time unless a software timer callback function is actually executing.
 *
 */
class SoftwareTimer {
public:
    SoftwareTimer();
    ~SoftwareTimer();

    void start(TickType_t delay = 0);
    void stop();
    void reset(TickType_t delay = 0);
    TickType_t period();

    void attachCallback(Callback<void()> func, TickType_t period, bool repeated);

private:
    TimerHandle_t handle;
    Callback<void()> _callback;

    static void callbackHandler(TimerHandle_t xTimer);
};