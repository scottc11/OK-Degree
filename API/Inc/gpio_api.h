#pragma once

#include "api.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_hal_gpio_ex.h" // do you need this?

GPIO_TypeDef *enable_gpio_clock(PinName pin);
uint32_t gpio_get_pin(PinName pin);
GPIO_TypeDef *gpio_get_port(PinName);
void enable_adc_pin(PinName pin);

void set_pin_pull(GPIO_InitTypeDef *config, PinMode mode);

void gpio_config_input_capture(PinName pin);
GPIO_PinState gpio_read_pin(PinName pin);
void gpio_irq_set(PinName pin, IrqEvent event, bool enable);
IRQn_Type gpio_get_irq_line(PinName pin);