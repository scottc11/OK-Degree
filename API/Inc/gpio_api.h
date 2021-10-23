#pragma once

#include "api.h"

GPIO_TypeDef *enable_gpio_clock(PinName pin);
uint32_t get_pin_num(PinName pin);
GPIO_TypeDef *get_pin_port(PinName);
void enable_adc_pin(PinName pin);