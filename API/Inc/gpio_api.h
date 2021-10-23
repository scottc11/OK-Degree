#pragma once

#include "api.h"

GPIO_TypeDef *enable_gpio_clock(PinName pin);
void enable_adc_pin(PinName pin);
void pin_config_i2c(PinName pin);