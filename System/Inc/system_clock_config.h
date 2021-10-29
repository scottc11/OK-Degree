#pragma once

#include "stm32f4xx_hal.h"

#define HCLK_FREQ 180
#define SYSCLK_FREQ 180
#define APB1_PERIPHERAL_FREQ 45
#define APB1_TIM_FREQ 90
#define APB2_PERIPHERAL_FREQ 90
#define APB2_TIM_FREQ 180

#ifdef __cplusplus
extern "C"
{
#endif

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif