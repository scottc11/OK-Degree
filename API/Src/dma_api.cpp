#include "dma_api.h"

void dma2_init() {
    /* DMA controller clock enable */
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/* ADC DMA Init */
void configure_adc_dma(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma_adc)
{
    __HAL_RCC_ADC1_CLK_ENABLE(); // mandatory

    hdma_adc->Instance = DMA2_Stream0;
    hdma_adc->Init.Channel = DMA_CHANNEL_0;
    hdma_adc->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc->Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc->Init.Mode = DMA_CIRCULAR;
    hdma_adc->Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(hdma_adc);
    
    __HAL_LINKDMA(hadc, DMA_Handle, *hdma_adc); // NOTE: DMA_Handle is a global supplied by HAL
}