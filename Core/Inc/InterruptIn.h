#pragma once

#include "main.h"
#include "gpio_api.h"
#include "Callback.h"


using namespace mbed;

class InterruptIn {
public:

    PinName _pin;
    PinMode _pull;
    GPIO_TypeDef *_port;
    uint32_t _pin_num;
    IrqEvent _event;
    IRQn_Type _irq;
    Callback<void()> riseCallback;
    Callback<void()> fallCallback;

    InterruptIn(PinName pin)
    {
        _pin = pin;
        _pull = PullNone; // default
        init();
    }

    InterruptIn(PinName pin, PinMode mode) {
        _pin = pin;
        _pull = mode;
        init();
    }

    void init();
    int read();
    void handleInterupt();
    void mode(PinMode mode);
    void rise(Callback<void()> func);
    void fall(Callback<void()> func);
    void gpio_irq_init(PinName pin);
    static void RouteCallback(uint16_t GPIO_Pin);

private:
    static InterruptIn *_instances[NUM_GPIO_IRQ_INSTANCES];
};