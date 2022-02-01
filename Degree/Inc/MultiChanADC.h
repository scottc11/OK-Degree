#pragma once

#include "main.h"
#include "tim_api.h"
#include "AnalogHandle.h"
#include "logger.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;

#ifndef ADC_DMA_BUFF_SIZE
#define ADC_DMA_BUFF_SIZE 8
#endif

#ifndef ADC_TIM_PRESCALER
#define ADC_TIM_PRESCALER 100
#endif

#ifndef ADC_TIM_PERIOD
#define ADC_TIM_PERIOD 2000
#endif

#define ADC_SAMPLE_RATE_HZ 2000

extern TIM_HandleTypeDef htim3;

void multi_chan_adc_init();
void multi_chan_adc_start();

uint32_t multi_chan_adc_get_sample_rate(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim);
void multi_chan_adc_set_sample_rate(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim, uint32_t sample_rate_hz);

void multi_chan_adc_enable_irq();
void multi_chan_adc_disable_irq();

void MX_ADC1_Init(void);
void MX_TIM3_Init(void);
void MX_DMA_Init(void);

void ADC1_DMA_Callback(uint16_t values[]);