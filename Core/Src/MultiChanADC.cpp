#include "MultiChanADC.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim3;

uint16_t DMA_ADC_BUFFER[ADC_DMA_BUFF_SIZE];

void multi_chan_adc_init()
{
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_TIM3_Init();
}

void multi_chan_adc_start()
{
    HAL_TIM_Base_Start(&htim3);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)DMA_ADC_BUFFER, ADC_DMA_BUFF_SIZE);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC1_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN ADC1_MspInit 0 */

    /* USER CODE END ADC1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC1 GPIO Configuration
  PA4     ------> ADC1_IN4
  PA5     ------> ADC1_IN5
  PA6     ------> ADC1_IN6
  PA7     ------> ADC1_IN7
  PC4     ------> ADC1_IN14
  PC5     ------> ADC1_IN15
  PB0     ------> ADC1_IN8
  PB1     ------> ADC1_IN9
  */
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */
    /**
 * NOTE: Important to set EOCSelection to "EOC Flag at the end of all converions" if you want to continuously read the ADC, otherwise
 *       the ADC will only do a single conversion 
*/
    /* USER CODE END ADC1_Init 1 */
    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
*/
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = ENABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 8;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    HAL_ADC_Init(&hadc1);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_6;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_7;
    sConfig.Rank = 2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_15;
    sConfig.Rank = 3;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_14;
    sConfig.Rank = 4;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_4;
    sConfig.Rank = 5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = 6;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_8;
    sConfig.Rank = 7;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
*/
    sConfig.Channel = ADC_CHANNEL_9;
    sConfig.Rank = 8;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM3_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 100;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 2000;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim3);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig);
}

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**
 * @brief Overload this function in 
*/
__WEAK void ADC1_DMA_Callback(uint16_t values[])
{
    // function to be overloaded in main program
}

/**
  * @brief This function handles DMA2 stream0 global interrupt.
  * NOTE: This must be declared when using an interupt peripheral.
  * When there is an interrupt for which no handler exists, "Default_Handler" will be called and put the system in an inifinite loop
  */
extern "C" void DMA2_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

/**
  * @brief  Regular conversion complete callback in non blocking mode 
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
  */
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        ADC1_DMA_Callback(DMA_ADC_BUFFER);
    }
}
