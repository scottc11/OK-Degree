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

