#include "logger.h"

UART_HandleTypeDef huart3;

void logger_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    gpio_enable_clock(OK_UART_RX);

    GPIO_InitStruct.Pin = gpio_get_pin(OK_UART_RX);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(gpio_get_port(OK_UART_RX), &GPIO_InitStruct);

    gpio_enable_clock(OK_UART_TX);

    GPIO_InitStruct.Pin = gpio_get_pin(OK_UART_TX);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(gpio_get_port(OK_UART_TX), &GPIO_InitStruct);

    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart3);
}

void logger_log(char *str)
{
    uart_transmit(str);
}

void logger_log(int const num) {
    uart_transmit(num);
}

void logger_log(float const f) {
    char str[33];
    utoa(f, str, 10);
    // snprintf(str, sizeof(str), "%f", f);
    HAL_UART_Transmit(&huart3, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

void logger_log_err(char *str)
{
    uart_transmit(str);
}

void uart_transmit(char const data[])
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), HAL_MAX_DELAY);
}

void uart_transmit(int const data)
{
    char tmp[33];
    itoa(data, tmp, 10);
    HAL_UART_Transmit(&huart3, (uint8_t *)tmp, strlen(tmp), HAL_MAX_DELAY);
}