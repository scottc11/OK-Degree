#include "InterruptIn.h"

InterruptIn *InterruptIn::_instances[NUM_GPIO_IRQ_INSTANCES] = {0};

void InterruptIn::init() {
    for (int i = 0; i < NUM_GPIO_IRQ_INSTANCES; i++)
    {
        if (_instances[i] == NULL)
        {
            _instances[i] = this;
            break;
        }
    }
}

/**
 * @brief set pin as PullUp, PullDown, or PullNone
*/ 
void InterruptIn::mode(PinMode mode) {
    _pull = mode;
    gpio_irq_init(_pin);
}

void InterruptIn::rise(Callback<void()> func)
{
    if (func) {
        riseCallback = func;
    }
    _mode = TriggerMode::Rising;
    gpio_irq_init(_pin);
}
void InterruptIn::fall(Callback<void()> func)
{
    if (func)
    {
        fallCallback = func;
    }
    _mode = TriggerMode::Falling;
    gpio_irq_init(_pin);
}

void InterruptIn::handleInterupt() {
    if (this->_mode == TriggerMode::Rising) {
        if (riseCallback) {
            riseCallback();
        }
    } else if (this->_mode == TriggerMode::Falling) {
        if (fallCallback) {
            fallCallback();
        }
    }
}

void InterruptIn::gpio_irq_init(PinName pin)
{
    _port = enable_gpio_clock(pin);
    _pin_num = get_pin_num(pin);

    // configure gpio for interupt
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = _pin_num;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    set_pin_pull(&GPIO_InitStruct, _pull);

    switch (_mode) {
        case TriggerMode::Rising:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
            break;
        case TriggerMode::Falling:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
            break;
        case TriggerMode::RiseFall:
            GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
            break;
    }

    HAL_GPIO_Init(_port, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

void InterruptIn::RouteCallback(uint16_t GPIO_Pin)
{
    for (auto ins : _instances)
    {
        if (ins && ins->_pin_num == GPIO_Pin)
        {
            ins->handleInterupt();
        }
    }
}

extern "C" void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    InterruptIn::RouteCallback(GPIO_Pin);
}