#include "DigitalOut.h"


void DigitalOut::gpio_init(PinName pin)
{
    if (pin == PinName::NC)
    {
        return;
    }

    // TODO: you always need to enable PortH, so move it to system config
    enable_gpio_clock(PortH);
    
    // enable gpio clock
    GPIO_TypeDef *port = enable_gpio_clock(STM_PORT(pin));
    uint32_t pin_num = gpio_pin_map[STM_PIN(pin)];

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(port, pin_num, GPIO_PIN_RESET);

    /*Configure GPIO pin : PA1 */
    GPIO_InitStruct.Pin = pin_num;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/**
 * @brief enables the given GPIO Ports Clock
 * @return the pointer to the given ports GPIO_TypeDef
*/
GPIO_TypeDef* DigitalOut::enable_gpio_clock(uint32_t port)
{
    switch (port) {
        case PortA:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            return GPIOA;
        case PortB:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            return GPIOB;
        case PortC:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            return GPIOC;
        case PortH:
            __HAL_RCC_GPIOH_CLK_ENABLE();
            return GPIOH;
        default:
            return (GPIO_TypeDef *) 0;
    }
}