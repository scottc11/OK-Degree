#pragma once

#include "common.h"
#include "tim_api.h"
#include "Callback.h"

#ifndef PPQN
#define PPQN 96
#endif

#ifndef EXT_CLOCK_INPUT
#define EXT_CLOCK_INPUT PA_3
#endif

#ifndef MAX_CLOCK_FREQ
#define MAX_CLOCK_FREQ 500
#endif

#ifndef MIN_CLOCK_FREQ
#define MIN_CLOCK_FREQ 65535
#endif

extern TIM_HandleTypeDef htim1; // 16-bit timer
extern TIM_HandleTypeDef htim2; // 32-bit timer

extern "C" void TIM1_UP_TIM10_IRQHandler(void);
extern "C" void TIM2_IRQHandler(void);

class SuperClock {
public:

    int tick;               // increments every time TIM1 overflows
    int pulse;
    int step;
    uint16_t ticksPerStep;  // how many TIM2 ticks per one step / quarter note
    uint16_t ticksPerPulse; // how many TIM2 ticks for one PPQN

    Callback<void()> tickCallback;           // this callback gets executed at a frequency equal to tim1_freq
    Callback<void()> input_capture_callback; // this callback gets executed every on the rising edge of external input
    Callback<void()> ppqnCallback;           // this callback gets executed at a rate equal to input capture / PPQN
    Callback<void()> resetCallback;
    Callback<void()> overflowCallback;       // callback executes when all when a full step completes

    /**
     * TODO: initial inputCapture value should be the product of TIM1 and TIM2 prescaler values combined with 120 BPM
     * so that the sequencer always gets initialized at 120 bpm, no matter the speed of the timers
    */
    SuperClock()
    {
        instance = this;
        ticksPerStep = 11129;
        ticksPerPulse = ticksPerStep / PPQN;
    };


    void initTIM1(uint16_t prescaler, uint16_t period);
    void initTIM2(uint16_t prescaler, uint16_t period);
    void start();

    void setFrequency(uint16_t freq);

    // Callback Setters
    void attach_tim1_callback(Callback<void()> func);
    void attachInputCaptureCallback(Callback<void()> func);
    void attachPPQNCallback(Callback<void()> func);
    void attachResetCallback(Callback<void()> func);
    
    // Low Level HAL interupt handlers
    void handleInputCaptureCallback();
    void handleTickCallback();
    static void RouteCallback(TIM_HandleTypeDef *htim);
    static void RouteCaptureCallback(TIM_HandleTypeDef *htim);

private:
    static SuperClock *instance;
    uint32_t tim1_freq;
    uint32_t tim2_freq;
};