#pragma once

#include "main.h"
#include "AnalogHandle.h"

#ifndef ADC_DMA_BUFF_SIZE
#define ADC_DMA_BUFF_SIZE 8
#endif

void multi_chan_adc_init();
void multi_chan_adc_start();
void MX_ADC1_Init(void);
void MX_TIM3_Init(void);
void MX_DMA_Init(void);

void ADC1_DMA_Callback(uint16_t values[]);