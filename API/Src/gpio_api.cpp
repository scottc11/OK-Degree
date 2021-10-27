#include "gpio_api.h"

/**
 * @brief enables the given GPIO Ports Clock
 * @return the pointer to the given ports GPIO_TypeDef
*/
GPIO_TypeDef * enable_gpio_clock(PinName pin)
{
    uint32_t port = STM_PORT(pin);
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
            return (GPIO_TypeDef *)0;
    }
}

uint32_t gpio_get_pin(PinName pin)
{
    return gpio_pin_map[STM_PIN(pin)];
}

GPIO_TypeDef * gpio_get_port(PinName pin) {
    uint32_t port = STM_PORT(pin);
    switch (port) {
        case PortA:
            return GPIOA;
        case PortB:
            return GPIOB;
        case PortC:
            return GPIOC;
        case PortH:
            return GPIOH;
        default:
            return (GPIO_TypeDef *)0;
    }
}

void enable_adc_pin(PinName pin)
{
    // enable gpio clock
    GPIO_TypeDef *port = enable_gpio_clock(pin);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = gpio_get_pin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void set_pin_pull(GPIO_InitTypeDef *config, PinMode mode) {
    switch (mode)
    {
    case PullUp:
        config->Pull = GPIO_PULLUP;
        break;
    case PullDown:
        config->Pull = GPIO_PULLDOWN;
        break;
    case PullNone:
        config->Pull = GPIO_NOPULL;
        break;
    }
}

GPIO_PinState gpio_read_pin(PinName pin)
{
    return HAL_GPIO_ReadPin(gpio_get_port(pin), gpio_get_pin(pin));
}

void gpio_irq_set(PinName pin, IrqEvent event, bool enable)
{
    /*  Enable / Disable Edge triggered interrupt and store event */
    if (event == IrqEvent::IRQ_EVENT_RISE)
    {
        if (enable)
        {
            LL_EXTI_EnableRisingTrig_0_31(1 << STM_PIN(pin));
        }
        else
        {
            LL_EXTI_DisableRisingTrig_0_31(1 << STM_PIN(pin));
        }
    }
    if (event == IrqEvent::IRQ_EVENT_FALL)
    {
        if (enable)
        {
            LL_EXTI_EnableFallingTrig_0_31(1 << STM_PIN(pin));
        }
        else
        {
            LL_EXTI_DisableFallingTrig_0_31(1 << STM_PIN(pin));
        }
    }
}