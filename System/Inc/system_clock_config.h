#pragma once

#include "stm32f4xx_hal.h"

#define HCLK_MHZ 180
#define SYSCLK_MHZ 180
#define APB1_PERIPHERAL_CLK 45
#define APB1_TIM_CLK 90
#define APB2_PERIPHERAL_CLK 90
#define APB2_TIM_CLK 180

#ifdef __cplusplus
extern "C"
{
#endif

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif