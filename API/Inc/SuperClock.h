#pragma once

#include "common.h"
#include "tim_api.h"
#include "Callback.h"

#ifndef PPQN
#define PPQN 96
#endif

extern TIM_HandleTypeDef htim1; // 16-bit timer
extern TIM_HandleTypeDef htim2; // 32-bit timer

extern "C" void TIM1_UP_TIM10_IRQHandler(void);
extern "C" void TIM2_IRQHandler(void);

class SuperClock {
public:
    
    volatile int counter;
    uint16_t inputCapture;
    uint16_t ticksPerStep; // how many TIM2 ticks for one PPQN

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
        inputCapture = 11129;
        ticksPerStep = inputCapture / PPQN;
    };

    void initTIM1(uint16_t prescaler, uint16_t period);
    void initTIM2(uint16_t prescaler, uint16_t period);
    void start();
    void attach_tim1_callback(Callback<void()> func);
    void attachInputCaptureCallback(Callback<void()> func);
    void attachPPQNCallback(Callback<void()> func);
    void handleInputCaptureCallback();
    void handleTickCallback();
    static void RouteCallback(TIM_HandleTypeDef *htim);
    static void RouteCaptureCallback(TIM_HandleTypeDef *htim);

private:
    static SuperClock *instance;
    uint32_t tim1_freq;
    uint32_t tim2_freq;
};