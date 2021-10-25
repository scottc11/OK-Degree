#pragma once

#include "main.h"
#include "gpio_api.h"
#include "Callback.h"

// for some reason, you need to declare this function in the header file as extern "C" for it to work
extern "C" void EXTI3_IRQHandler(void);

using namespace mbed;

class InterruptIn {
public:
    enum class TriggerMode
    {
        Rising = GPIO_MODE_IT_RISING,
        Falling = GPIO_MODE_IT_FALLING,
        RiseFall = GPIO_MODE_IT_RISING_FALLING
    };

    PinName _pin;
    GPIO_TypeDef *_port;
    uint32_t _pin_num;
    TriggerMode _mode;
    Callback<void()> riseCallback;
    Callback<void()> fallCallback;

    InterruptIn(PinName pin)
    {
        _pin = pin;
        for (int i = 0; i < NUM_GPIO_IRQ_INSTANCES; i++)
        {
            if (_instances[i] == NULL) {
                _instances[i] = this;
                break;
            }
        }
    }

    void handleInterupt();
    void rise(Callback<void()> func);
    void fall(Callback<void()> func);
    void gpio_irq_init(PinName pin);

    static void RouteCallback(uint16_t GPIO_Pin);

private:
    static InterruptIn *_instances[NUM_GPIO_IRQ_INSTANCES];
};