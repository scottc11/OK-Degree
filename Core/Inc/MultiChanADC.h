#pragma once

#include "main.h"

template <int N>
class MultiChanADC {
public:

    uint16_t dmaBuffer[N]; // this may need to be a pointer to an external constant
    PinName adcPins[N];

    /**
     * @param pins and array of PinName values 
    */
    MultiChanADC(PinName pins[N]) {
        for (int i = 0; i < N; i++)
        {
            adcPins[i] = pins[i];
        }
    }

    void init() {
        TIM3_Init();
        ADC1_Init();
    }

    void start() {
        HAL_TIM_Base_Start(&_htim);
    }

    void ADC1_Init() {

        __HAL_RCC_ADC1_CLK_ENABLE();
        
        /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) */
        _hadc.Instance = ADC1;
        _hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
        _hadc.Init.Resolution = ADC_RESOLUTION_12B;
        _hadc.Init.ScanConvMode = ENABLE;
        _hadc.Init.ContinuousConvMode = DISABLE;
        _hadc.Init.DiscontinuousConvMode = DISABLE;
        _hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
        _hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
        _hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        _hadc.Init.NbrOfConversion = 8;
        _hadc.Init.DMAContinuousRequests = ENABLE;
        _hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV; // Important to set EOCSelection to "EOC Flag at the end of all converions" if you want to continuously read the ADC, otherwise the ADC will only do a single conversion
        HAL_ADC_Init(&_hadc);

        /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. */
        ADC_ChannelConfTypeDef sConfig = {0};
        sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
        for (int i = 0; i < N; i++)
        {
            for (auto p : ADC_PIN_MAP)
            {
                if (p.pin == adcPins[i])
                {
                    sConfig.Channel = p.channel;
                    break;
                }
            }
            sConfig.Rank = i + 1;          // rank the pin based on the order in which it occurs in adcPins array
            HAL_ADC_ConfigChannel(&_hadc, &sConfig);
        }
        
    }

    void TIM3_Init() {
        __HAL_RCC_TIM3_CLK_ENABLE();

        TIM_ClockConfigTypeDef sClockSourceConfig = {0};
        TIM_MasterConfigTypeDef sMasterConfig = {0};

        _htim.Instance = TIM3;
        _htim.Init.Prescaler = 1;
        _htim.Init.CounterMode = TIM_COUNTERMODE_UP;
        _htim.Init.Period = 2000;
        _htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        _htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        HAL_TIM_Base_Init(&_htim);

        sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
        HAL_TIM_ConfigClockSource(&_htim, &sClockSourceConfig);

        sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
        sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
        HAL_TIMEx_MasterConfigSynchronization(&_htim, &sMasterConfig);
    }

    ADC_HandleTypeDef _hadc;
    DMA_HandleTypeDef _hdma;
    TIM_HandleTypeDef _htim;
};