#include "SuperClock.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

SuperClock *SuperClock::instance = NULL;

void SuperClock::start()
{
    HAL_StatusTypeDef status;
    status = HAL_TIM_Base_Start_IT(&htim1);
    error_handler(status);
    status = HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4);
    error_handler(status);
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

/**
 * @brief initialize TIM2 as a slave to TIM1
 * @param prescaler setting to 1 should be best
 * @param period setting to 65535 should be best
*/
void SuperClock::initTIM2(uint16_t prescaler, uint16_t period)
{
    tim2_freq = tim1_freq / (prescaler * period);

    __HAL_RCC_TIM2_CLK_ENABLE(); // turn on timer clock

    gpio_config_input_capture(EXT_CLOCK_INPUT); // config PA3 in input capture mode

    /* TIM2 interrupt Init */
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    TIM_SlaveConfigTypeDef sSlaveConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_IC_InitTypeDef sConfigIC = {0};
    HAL_StatusTypeDef status;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = prescaler;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = period;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    status = HAL_TIM_Base_Init(&htim2);
    if (status != HAL_OK)
        error_handler(status);

    status = HAL_TIM_IC_Init(&htim2);
    if (status != HAL_OK)
        error_handler(status);

    sSlaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
    sSlaveConfig.InputTrigger = TIM_TS_ITR0;
    status = HAL_TIM_SlaveConfigSynchro(&htim2, &sSlaveConfig);
    if (status != HAL_OK)
        error_handler(status);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    status = HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
    if (status != HAL_OK)
        error_handler(status);

    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    status = HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_4);
    if (status != HAL_OK)
        error_handler(status);
}

void SuperClock::attach_tim1_callback(Callback<void()> func)
{
    tickCallback = func;
}

void SuperClock::attachInputCaptureCallback(Callback<void()> func) {
    input_capture_callback = func;
}

void SuperClock::attachPPQNCallback(Callback<void(uint8_t pulse)> func)
{
    ppqnCallback = func;
}

void SuperClock::attachResetCallback(Callback<void()> func) {
    resetCallback = func;
}

void SuperClock::setFrequency(uint16_t freq) {
    if (freq < MAX_CLOCK_FREQ) {
        ticksPerStep = MAX_CLOCK_FREQ;
    } else if (freq > MIN_CLOCK_FREQ) {
        ticksPerStep = MIN_CLOCK_FREQ;
    } else {
        ticksPerStep = freq;
    }
    
    ticksPerPulse = (ticksPerStep * 2) / PPQN; // Forget why we multiply by 2
}

void SuperClock::handleInputCaptureCallback()
{
    // if pulse != 0, 
    // while (pulse != 0)
    //   ppqnCallback(pulse);
    //   pulse++;

    tick = 0; // Reset the clock tick zero, so it will trigger the clock output in the period elapsed loop callback
    pulse = 0;

    __HAL_TIM_SetCounter(&htim2, 0); // reset after each input capture
    uint16_t inputCapture = __HAL_TIM_GetCompare(&htim2, TIM_CHANNEL_4);
    setFrequency(inputCapture);

    // if (input_capture_callback)
    // {
    //     input_capture_callback();
    // }
}

/**
 * @brief this callback gets called everytime TIM1 overflows.
 * 
 * Use this callback to advance the sequencer position by 1 everytime tick equals the 
 * calculated PPQN value via TIM2 Input capture callback.
*/ 
void SuperClock::handleTickCallback()
{

    if (tick < ticksPerPulse) { // make sure you don't loose a step by not using "<=" instead of "<"
        if (tick == 0) { // handle first tick in step
            if (ppqnCallback) ppqnCallback(pulse);
            if (pulse == 0)
            {
                if (resetCallback)
                    resetCallback();
            }
        }
        tick++;
    }
    // this block will continue the sequence when an external clock is not detected, 
    // otherwise input capture will reset tick back to 0
    else {
        tick = 0;

        if (pulse < PPQN - 1) {
            pulse++;
        } else {
            pulse = 0;
        }
    }

    if (tickCallback)
    {
        tickCallback();
    }
}

void SuperClock::RouteCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim1)
    {
        instance->handleTickCallback();
    }
}

/**
 * The key in this callback is to reset the sequencer and/or clock step to 0 (ie. step 1)
 * This way, the sequence will always be in sync with the beat because the rising edge of the external tempo signal will always line up to tick 0 of the sequence.
 * 
 * The caveat being we risk losing ticks near the end of each external clock signal, because the callback could execute before the final few ticks have a chance to be executed.
 * This could cause issues if there is important sequence data in those last remaining ticks, such as setting a trigger output high or low.
 * 
 * To remedy this problem, we could keep track of how many ticks have executed in the sequence for each clock signal, so that if the HAL_TIM_IC_CaptureCallback executes prior to 
 * the last tick in the sequence, we could quickly execute all those remaining ticks in rapid succession - which could sound weird, but likely neccessary.
 * 
 * NOTE: The downfall of this approach is that some steps of the sequence will be missed when the external tempo signal increases,
 * but this should be a more musical way to keeping things in sync.
 * 
 * NOTE: When jumping from a really fast tempo to a really slow tempo, the sequence steps will progress much faster than the incoming tempo (before the IC Calculation has time to update the sequencer pulse duration)
 * This means the sequence could technically get several beats ahead of any other gear.
 * To handle this, you could prevent the next sequence step from occuring if all sub-steps of the current step have been executed prior to a new IC event
 */
void SuperClock::RouteCaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        instance->handleInputCaptureCallback();
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
  * @brief This function handles TIM2 global interrupt.
*/
extern "C" void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

/**
 * @brief This callback handles all TIMx overflow interupts (if TIMx was configured in interupt mode)
 * Ideally, you would use this to manage a global / system tick value, which then gets devided down
 * to handle events at a lower frequency.
*/
extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    SuperClock::RouteCallback(htim);
}

/**
 * @brief Input Capture Callback for all TIMx configured in Input Capture mode
*/ 
extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    SuperClock::RouteCaptureCallback(htim);
}