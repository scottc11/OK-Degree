#pragma once

#include "api.h"

void dma2_init();
void configure_adc_dma(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma_adc1);

