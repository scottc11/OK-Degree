#pragma once

#include "main.h"

class DigitalOut {
public:

    GPIO_TypeDef *_port;
    uint32_t _pin;

    DigitalOut(PinName pin) {
        gpio_init(pin);
    }

    DigitalOut &operator=(int value);

    void write(int value);
    int read();

    void gpio_init(PinName pin);
    GPIO_TypeDef* enable_gpio_clock(uint32_t port);
};