#include "tim_api.h"

/**
 * @brief Get the APB Timer Clock Frequency
 * 
 * @param htim 
 * @return uint32_t 
 */
uint32_t tim_get_APBx_freq(TIM_HandleTypeDef *htim) {
    uint32_t pclk;
    if (htim->Instance == TIM1)
    {
        pclk = HAL_RCC_GetPCLK1Freq();
    }
    else
    {
        pclk = HAL_RCC_GetPCLK2Freq();
    }
    return pclk * 2; // Timer clocks are always equal to PCLK * 2
}

/**
 * @brief Get the freqeuncy of TIMx
 * OverflowEvent = APBx / ((period + 1) * (prescaler + 1))
 * @param TIM_HandleTypeDef
 * @return uint32_t frequency
 */
uint32_t tim_get_overflow_freq(TIM_HandleTypeDef *htim)
{
    uint16_t prescaler = htim->Instance->PSC;
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(htim);
    
    uint32_t APBx_freq = tim_get_APBx_freq(htim);

    return APBx_freq / ((prescaler + 1) * (period + 1));
}

/**
 * @brief
 * period = APBx / (prescaler * target_frequency)
 *
 * TODO: when period gets set to less than 1, than target frequency is not possible given the current prescaler value.
 * So subtract the prescaler value by 1 and try again until target frequency can be achieved.
 *
 * @param htim
 * @param target_freq
 */
void tim_set_overflow_freq(TIM_HandleTypeDef *htim, uint32_t target_freq) {
    uint32_t period;
    uint16_t prescaler = htim->Instance->PSC;
    uint32_t APBx_freq = tim_get_APBx_freq(htim);
    period = APBx_freq / ((prescaler + 1) * target_freq);
    __HAL_TIM_SET_AUTORELOAD(htim, period);
}