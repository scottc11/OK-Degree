#pragma once

#include "main.h"

class DigitalOut {
public:
    DigitalOut(PinName pin) {
        gpio_init(pin);
    }

    void gpio_init(PinName pin);
    GPIO_TypeDef* enable_gpio_clock(uint32_t port);
};