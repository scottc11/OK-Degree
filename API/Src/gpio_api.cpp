#include "gpio_api.h"

/**
 * @brief enables the given GPIO Ports Clock
 * @return the pointer to the given ports GPIO_TypeDef
*/
GPIO_TypeDef * gpio_enable_clock(PinName pin)
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
    GPIO_TypeDef *port = gpio_enable_clock(pin);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = gpio_get_pin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void gpio_config_input_capture(PinName pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_TypeDef *port = gpio_enable_clock(pin);
    GPIO_InitStruct.Pin = gpio_get_pin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
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

/**
 * @brief Use HAL LL to set pin speed after initialization
*/ 
void gpio_set_pin_speed(PinName pin, GPIO_Speed speed)
{
    LL_GPIO_SetPinSpeed(gpio_get_port(pin), gpio_get_pin(pin), speed);
}

GPIO_PinState gpio_read_pin(PinName pin)
{
    return HAL_GPIO_ReadPin(gpio_get_port(pin), gpio_get_pin(pin));
}

/**
 * @brief   Enable / Disable Edge triggered interrupt via LL Interupt Registers
*/
void gpio_irq_set(PinName pin, IrqEvent event, bool enable)
{
    
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

IRQn_Type gpio_get_irq_line(PinName pin)
{
    uint32_t pin_num = gpio_get_pin(pin);
    switch (pin_num)
    {
    case GPIO_PIN_0:
        return EXTI0_IRQn;
    case GPIO_PIN_1:
        return EXTI1_IRQn;
    case GPIO_PIN_2:
        return EXTI2_IRQn;
    case GPIO_PIN_3:
        return EXTI3_IRQn;
    case GPIO_PIN_4:
        return EXTI4_IRQn;
    case GPIO_PIN_5:
        return EXTI9_5_IRQn;
    case GPIO_PIN_6:
        return EXTI9_5_IRQn;
    case GPIO_PIN_7:
        return EXTI9_5_IRQn;
    case GPIO_PIN_8:
        return EXTI9_5_IRQn;
    case GPIO_PIN_9:
        return EXTI9_5_IRQn;
    case GPIO_PIN_10:
        return EXTI15_10_IRQn;
    case GPIO_PIN_11:
        return EXTI15_10_IRQn;
    case GPIO_PIN_12:
        return EXTI15_10_IRQn;
    case GPIO_PIN_13:
        return EXTI15_10_IRQn;
    case GPIO_PIN_14:
        return EXTI15_10_IRQn;
    case GPIO_PIN_15:
        return EXTI15_10_IRQn;
    }
}