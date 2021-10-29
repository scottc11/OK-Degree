#pragma once

#include "common.h"
#include "tim_api.h"
#include "Callback.h"

extern TIM_HandleTypeDef htim1; // 16-bit timer
extern TIM_HandleTypeDef htim2; // 32-bit timer

extern "C" void TIM1_UP_TIM10_IRQHandler(void);

class SuperClock {
public:
    
    uint32_t tim1_freq;
    Callback<void()> tim1_overflow_callback; // this callback gets executed at a frequency equal to tim1_freq

    SuperClock() {
        instance = this;
    };

    void initTIM1(uint16_t prescaler, uint16_t period);
    void start();
    void attach_tim1_callback(Callback<void()> func);
    static void RouteCallback(TIM_HandleTypeDef *htim);

private:
    static SuperClock *instance;
};