#pragma once

#include "stm32f4xx_hal.h"

#define HCLK_FREQ 180000000            // 180 Mhz
#define SYSCLK_FREQ 180000000          // 180 Mhz
#define APB1_PERIPHERAL_FREQ 45000000  // 45 Mhz
#define APB1_TIM_FREQ 90000000         // 90 Mhz
#define APB2_PERIPHERAL_FREQ 90000000  // 90 Mhz
#define APB2_TIM_FREQ 180000000        // 180 Mhz

#ifdef __cplusplus
extern "C"
{
#endif

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif