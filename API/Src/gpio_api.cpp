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

void enable_adc_pin(PinName pin)
{
    // enable gpio clock
    GPIO_TypeDef *port = enable_gpio_clock(pin);
    uint32_t pin_num = gpio_pin_map[STM_PIN(pin)];

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin_num;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void pin_config_i2c(PinName pin) {
    GPIO_TypeDef *port = enable_gpio_clock(pin);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
}