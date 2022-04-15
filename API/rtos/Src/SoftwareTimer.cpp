#include "SoftwareTimer.h"

SoftwareTimer::SoftwareTimer() {}

SoftwareTimer::~SoftwareTimer()
{
    detachCallback();
    xTimerDelete(handle, 1);
}

/**
 * @brief Start the software timer
 * @note Do not use a block time if calling a timer API function from a timer callback function, as doing so could cause a deadlock!
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

/**
 * @brief stops a timer that was previously started
 * @note Do not use a block time if calling a timer API function from a timer callback function, as doing so could cause a deadlock!
 */
void SoftwareTimer::stop(TickType_t delay /*= 0*/)
{
    xTimerStop(handle, delay);
}

/**
 * @brief re-starts a timer that was previously created using the xTimerCreate() API function.
 * @note If the timer had already been started and was already in the active state, then xTimerReset() will
 * cause the timer to re-evaluate its expiry time so that it is relative to when xTimerReset() was called.
 * @note If the timer was in the dormant state then xTimerReset() has equivalent functionality to the xTimerStart() API function.
 * @note Do not use a block time if calling a timer API function from a timer callback function, as doing so could cause a deadlock!
 */
void SoftwareTimer::reset(TickType_t delay /*= 0*/)
{
    xTimerReset(handle, delay);
}

TickType_t SoftwareTimer::getPeriod()
{
    return xTimerGetPeriod(handle);
}

/**
 * @brief change the period of an active or dormant state timer. Changing the period of a dormant timers will also start the timer.
 *
 * @param period
 *
 * @note Timer periods are specified in tick periods, so the constant portTICK_PERIOD_MS can be used to convert a time that has been specified in milliseconds.
 * For example, if the timer must expire after 100 ticks, then xNewPeriod should be set to 100. Alternatively, if the timer must expire after 500ms, then xNewPeriod can be set to ( 500 / portTICK_PERIOD_MS ) provided configTICK_RATE_HZ is less than or equal to 1000.
 */
void SoftwareTimer::setPeriod(TickType_t period)
{
    xTimerChangePeriod(handle, period, 100);
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

void SoftwareTimer::detachCallback()
{
    _callback = 0;
}

void SoftwareTimer::callbackHandler(TimerHandle_t xTimer)
{
    SoftwareTimer *timer = static_cast<SoftwareTimer *>(pvTimerGetTimerID(xTimer));
    if (timer->_callback)
    {
        timer->_callback();
    }   
}