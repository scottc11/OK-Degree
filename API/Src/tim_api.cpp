#include "tim_api.h"


/**
 * @brief Configure HAL TIM
 * Timer Overflow Frequency = APBx / ((prescaler + 1) * (period + 1))
*/
void tim_config(TIM_HandleTypeDef *htim, TIM_TypeDef *TIM, uint16_t prescaler, uint16_t period)
{
    
}