#include "tim_api.h"

/**
 * @brief Get the freqeuncy of TIMx
 * APBx / ((period + 1) * (prescaler + 1))
 * @param TIM_HandleTypeDef
 * @return uint32_t frequency
 */
uint32_t tim_get_overflow_freq(TIM_HandleTypeDef *htim)
{
    uint32_t pclk;
    uint16_t prescaler = htim->Instance->PSC;
    uint16_t period = __HAL_TIM_GET_AUTORELOAD(htim);
    
    if (htim->Instance == TIM1)
    {
        pclk = HAL_RCC_GetPCLK1Freq();
    } else {
        pclk = HAL_RCC_GetPCLK2Freq();
    }
    uint32_t APBx_freq = pclk * 2; // Timer clocks are always equal to PCLK * 2

    return APBx_freq / ((prescaler + 1)*(period + 1));
}