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
        prevValue = currValue;
        currValue = convert12to16(DMA_BUFFER[index]);
        
        if (filter) {
            currValue = (currValue * filterAmount) + (prevValue * (1 - filterAmount));
        }

        return invert ? BIT_MAX_16 - currValue : currValue;
    }

    void setFilter(float value) {
        if (value == 0) {
            filter = false;
        } else if (value > (float)1) {
            // raise error
            filter = false;
        }
        else {
            filterAmount = value;
            filter = true;
        }
    }

    void enableFilter() { filter = true; }
    void disableFilter() { filter = false; }

    void invertReadings() {
        this->invert = !this->invert;
    }

    static uint16_t DMA_BUFFER[ADC_DMA_BUFF_SIZE];
    static PinName ADC_PINS[ADC_DMA_BUFF_SIZE];

private:
    uint16_t currValue;
    uint16_t prevValue;
    bool filter = false;
    float filterAmount = 0.1;
    bool invert = false;
};