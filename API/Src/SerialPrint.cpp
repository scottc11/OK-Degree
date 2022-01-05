#include "SerialPrint.h"


/**
 * @brief Initializes UART3 peripheral
 * 
 */
void SerialPrint::init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    gpio_enable_clock(rx_pin);

    GPIO_InitStruct.Pin = gpio_get_pin(rx_pin);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(gpio_get_port(rx_pin), &GPIO_InitStruct);

    gpio_enable_clock(tx_pin);

    GPIO_InitStruct.Pin = gpio_get_pin(tx_pin);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(gpio_get_port(tx_pin), &GPIO_InitStruct);

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

void SerialPrint::transmit(char data[])
{
    HAL_UART_Transmit(&_huart, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);
}

void SerialPrint::transmit(int data)
{
    char tmp[33];
    itoa(data, tmp, 10);
    HAL_UART_Transmit(&_huart, (uint8_t *)tmp, strlen(tmp), HAL_MAX_DELAY);
}