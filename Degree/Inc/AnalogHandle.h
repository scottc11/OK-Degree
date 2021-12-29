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

    uint16_t read_u16();
    void setFilter(float value);

    uint16_t samplePeakToPeak(int numSamples);
    uint16_t getZeroCrossing();
    uint16_t getMax();
    uint16_t getMin();

    uint16_t getSampleFrequency();

    static uint16_t DMA_BUFFER[ADC_DMA_BUFF_SIZE];
    static PinName ADC_PINS[ADC_DMA_BUFF_SIZE];
    static uint16_t tim_prescaler;
    static uint16_t tim_period;
    static bool sample_ready;

private:
    uint16_t currValue;
    uint16_t prevValue;
    bool filter = false;
    float filterAmount = 0.1;
    uint32_t sampleCounter;
    uint16_t min;
    uint16_t max;
    uint16_t zeroCrossing;
};