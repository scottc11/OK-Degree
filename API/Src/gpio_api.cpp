#include "gpio_api.h"

/**
 * @brief enables the given GPIO Ports Clock
 * @return the pointer to the given ports GPIO_TypeDef
*/
GPIO_TypeDef * enable_gpio_clock(uint32_t port)
{
    switch (port)
    {
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
        return (GPIO_TypeDef *)0;
    }
}

void enable_adc_pin(PinName pin)
{
    // enable gpio clock
    GPIO_TypeDef *port = enable_gpio_clock(STM_PORT(pin));
    uint32_t pin_num = gpio_pin_map[STM_PIN(pin)];

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin_num;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}