#include "SuperClock.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

SuperClock *SuperClock::instance = NULL;

void SuperClock::start()
{
    HAL_TIM_Base_Start_IT(&htim1);
}

    /**
 * @brief Configure TIM1 in Master Mode
 * Timer Overflow Frequency = APB2 / ((prescaler) * (period))
 * 
 * @param prescaler none-zero indexed clock prescaler value
 * @param period none-zero indexed timer period value
*/
    void SuperClock::initTIM1(uint16_t prescaler, uint16_t period)
{
    __HAL_RCC_TIM1_CLK_ENABLE();

    /* TIM1 interrupt Init */
    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    HAL_StatusTypeDef status;

    tim1_freq = APB2_TIM_FREQ / (prescaler * period);

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = prescaler - 1;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = period - 1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&htim1);
    if (status != HAL_OK)
    {
        error_handler(status);
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    status = HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig);
    if (status != HAL_OK)
    {
        error_handler(status);
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
    status = HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig);
    if (status != HAL_OK)
    {
        error_handler(status);
    }
}

void SuperClock::attach_tim1_callback(Callback<void()> func)
{
    tim1_overflow_callback = func;
}

void SuperClock::RouteCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim1)
    {
        if (instance->tim1_overflow_callback) {
            instance->tim1_overflow_callback();
        }
    }
}

/**
  * @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
  */
extern "C" void TIM1_UP_TIM10_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim1);
}

/**
 * @brief This callback handles all TIMx overflow interupts (if TIMx was configured in interupt mode)
 * Ideally, you would use this to manage a global / system tick value, which then gets devided down
 * to handle events at a lower frequency.
*/
extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    SuperClock::RouteCallback(htim);
}