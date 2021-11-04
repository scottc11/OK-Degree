#pragma once

#include "main.h"

class AnalogHandle {
public:
    AnalogHandle(int _index) {
        index = _index;
    }

    int index;

    uint16_t read_u16() {
        return DMA_BUFFER[index];
    }

    static uint16_t DMA_BUFFER[ADC_DMA_BUFF_SIZE];
};