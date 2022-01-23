#pragma once

#include "common.h"

uint32_t tim_get_APBx_freq(TIM_HandleTypeDef *htim);

uint32_t tim_get_overflow_freq(TIM_HandleTypeDef *htim);

void tim_set_overflow_freq(TIM_HandleTypeDef *htim, uint32_t target_freq);