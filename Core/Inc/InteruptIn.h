#pragma once

#include "main.h"
#include "gpio_api.h"

// for some reason, you need to declare this function in the header file as extern "C" for it to work
extern "C" void EXTI3_IRQHandler(void);

class InteruptIn {
public:
    
    GPIO_TypeDef *_port;
    uint32_t _pin;

    InteruptIn(PinName pin) {
        gpio_irq_init(pin);
        for (int i = 0; i < NUM_GPIO_IRQ_INSTANCES; i++)
        {
            if (_instances[i] == NULL) {
                _instances[i] = this;
                break;
            }
        }
    }

    void handleInterupt();

    void gpio_irq_init(PinName pin)
    {
        _port = enable_gpio_clock(STM_PORT(pin));
        _pin = gpio_pin_map[STM_PIN(pin)];
        
        // configure gpio for interupt
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = _pin;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(_port, &GPIO_InitStruct);

        /* EXTI interrupt init*/
        HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    }

    static void RouteCallback(uint16_t GPIO_Pin);

private:
    static InteruptIn *_instances[NUM_GPIO_IRQ_INSTANCES];
};