#pragma once

#include "main.h"

/**
 * @brief Simple class that pulls the data from a DMA buffer into an object
*/ 
class AnalogHandle {
public:
    AnalogHandle(int _index) {
        index = _index;
    }

    AnalogHandle(PinName pin) {
        // iterate over static member ADC_PINS and match index to pin
        for (int i = 0; i < ADC_DMA_BUFF_SIZE; i++)
        {
            if (pin == ADC_PINS[i]) {
                index = i;
                break;
            }
        }
    }

    int index;

    uint16_t read_u16() {
        return DMA_BUFFER[index];
    }

    static uint16_t DMA_BUFFER[ADC_DMA_BUFF_SIZE];
    static PinName ADC_PINS[ADC_DMA_BUFF_SIZE];
};