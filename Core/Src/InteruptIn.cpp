#include "InteruptIn.h"

InteruptIn *InteruptIn::_instances[NUM_GPIO_IRQ_INSTANCES] = {0};

void InteruptIn::handleInterupt() {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
}

void InteruptIn::RouteCallback(uint16_t GPIO_Pin)
{
    for (auto ins : _instances)
    {
        if (ins && ins->_pin == GPIO_Pin)
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
    InteruptIn::RouteCallback(GPIO_Pin);
}