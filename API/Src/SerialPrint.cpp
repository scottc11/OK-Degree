#include "SerialPrint.h"


/**
 * @brief Initializes UART3 peripheral
 * 
 */
void SerialPrint::init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    gpio_enable_clock(PC_11);
    gpio_enable_clock(PC_10);

    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    _huart.Instance = USART3;
    _huart.Init.BaudRate = 115200;
    _huart.Init.WordLength = UART_WORDLENGTH_8B;
    _huart.Init.StopBits = UART_STOPBITS_1;
    _huart.Init.Parity = UART_PARITY_NONE;
    _huart.Init.Mode = UART_MODE_TX;
    _huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    _huart.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&_huart);
}

void SerialPrint::transmit(char *data)
{
    HAL_UART_Transmit(&_huart, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);
}