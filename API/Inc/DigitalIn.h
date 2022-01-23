#pragma once

#include "common.h"

class DigitalIn {
public:
    GPIO_TypeDef *_port;
    uint32_t _pin;

    DigitalIn(PinName pin)
    {
        gpio_input_init(pin);
    }

    DigitalIn &operator=(int value);
    
    int read();

    void gpio_input_init(PinName pin);
};