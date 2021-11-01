#pragma once

#include "common.h"

class DigitalOut {
public:

    GPIO_TypeDef *_port;
    uint32_t _pin;

    DigitalOut(PinName pin) {
        gpio_init(pin, 0);
    }
    
    DigitalOut(PinName pin, int value)
    {
        gpio_init(pin, value);
    }

    DigitalOut &operator=(int value);

    void write(int value);
    int read();

    void gpio_init(PinName pin, int value);
};