#include "SoftwareTimer.h"

SoftwareTimer::SoftwareTimer() {}

SoftwareTimer::~SoftwareTimer()
{
    xTimerDelete(handle, 1);
}

/**
 * @brief Start the software timer
 * 
 * @note  If the timer had already been started and was already in the active state, then xTimerStart() 
 * has equivalent functionality to the xTimerReset() API function.
 */
void SoftwareTimer::start(TickType_t delay /*= 0*/)
{
    if (!xTimerIsTimerActive(handle))
    {
        xTimerStart(handle, delay);
    }
}

void SoftwareTimer::stop() {

}

void SoftwareTimer::reset(TickType_t delay /*= 0*/)
{
    xTimerReset(handle, delay);
}

TickType_t SoftwareTimer::period()
{
    return xTimerGetPeriod(handle);
}

/**
 * @brief attach a callback for the timer to execute
 * 
 * @param func 
 * @param period frequency
 * @param repeated one-shot or autoreload
 */
void SoftwareTimer::attachCallback(Callback<void()> func, TickType_t period, bool repeated)
{
    _callback = func;
    handle = xTimerCreate(
        "timer",                     // name for timer
        period,                      // period of timer in ticks
        repeated ? pdTRUE : pdFALSE, // auto-reload flag
        this,                        // unique ID for timer
        callbackHandler);            // callback function
}

void SoftwareTimer::callbackHandler(TimerHandle_t xTimer)
{
    SoftwareTimer *timer = static_cast<SoftwareTimer *>(pvTimerGetTimerID(xTimer));
    if (timer->_callback)
    {
        timer->_callback();
    }   
}