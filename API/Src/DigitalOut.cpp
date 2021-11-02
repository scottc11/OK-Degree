#include "DigitalOut.h"

/** A shorthand for write()
     * \sa DigitalOut::write()
     * @code
     *      DigitalIn  button(BUTTON1);
     *      DigitalOut led(LED1);
     *      led = button;   // Equivalent to led.write(button.read())
     * @endcode
     */
DigitalOut &DigitalOut::operator= (int value)
{
    write(value);
    return *this;
}

void DigitalOut::gpio_init(PinName pin, int value)
{
    if (pin == PinName::NC)
    {
        return;
    }
    
    // enable gpio clock
    _port = gpio_enable_clock(pin);
    _pin = gpio_pin_map[STM_PIN(pin)];

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(_port, _pin, (GPIO_PinState)value);

    /*Configure GPIO pin : PA1 */
    GPIO_InitStruct.Pin = _pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(_port, &GPIO_InitStruct);
}

void DigitalOut::write(int value) {
    if (value) {
        HAL_GPIO_WritePin(_port, _pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(_port, _pin, GPIO_PIN_RESET);
    }
}

int DigitalOut::read() {
    return HAL_GPIO_ReadPin(_port, _pin);
}