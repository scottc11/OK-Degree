#pragma once

#include "api.h"

GPIO_TypeDef *enable_gpio_clock(uint32_t port);
void enable_adc_pin(PinName pin);
