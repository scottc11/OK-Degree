#include "DigitalIn.h"

int DigitalIn::read()
{
    return HAL_GPIO_ReadPin(_port, _pin);
}

void DigitalIn::gpio_input_init(PinName pin)
{
    if (pin == PinName::NC)
    {
        return;
    }

    _port = gpio_enable_clock(pin);
    _pin = gpio_pin_map[STM_PIN(pin)];

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = _pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(_port, &GPIO_InitStruct);
}