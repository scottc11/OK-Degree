#pragma once

#include "common.h"
#include "tim_api.h"

extern TIM_HandleTypeDef htim1; // 16-bit timer
extern TIM_HandleTypeDef htim2; // 32-bit timer

extern "C" void TIM1_UP_TIM10_IRQHandler(void);

class SuperClock {
public:
    
    uint32_t tim1_freq;

    SuperClock(){};
    void initTIM1(uint16_t prescaler, uint16_t period);
};