#pragma once

#include "main.h"
#include "logger.h"
#define ADC_SAMPLE_COUNTER_LIMIT 100

/**
 * @brief Simple class that pulls the data from a DMA buffer into an object
*/ 
class AnalogHandle {
public:
    AnalogHandle(PinName pin) {
        // iterate over static member ADC_PINS and match index to pin
        for (int i = 0; i < ADC_DMA_BUFF_SIZE; i++)
        {
            if (pin == ADC_PINS[i]) {
                index = i;
                break;
            }
        }
        // Add constructed instance to the static list of instances (required for IRQ routing)
        for (int i = 0; i < ADC_DMA_BUFF_SIZE; i++)
        {
            if (_instances[i] == NULL)
            {
                _instances[i] = this;
                break;
            }
        }
    }

    int index;

    uint16_t read_u16() {
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
            prevValue = convert12to16(DMA_BUFFER[index]);
            filterAmount = value;
            filter = true;
        }
    }

    void enableFilter() { filter = true; }
    void disableFilter() { filter = false; }

    void invertReadings() {
        this->invert = !this->invert;
    }

    void calculateSignalNoise(uint16_t sample);

    void sampleReadyCallback(uint16_t sample);

    static void sampleReadyTask(void *params);
    static void RouteConversionCompleteCallback();

    static uint16_t DMA_BUFFER[ADC_DMA_BUFF_SIZE];
    static PinName ADC_PINS[ADC_DMA_BUFF_SIZE];
    static AnalogHandle *_instances[ADC_DMA_BUFF_SIZE];
    static SemaphoreHandle_t semaphore;

private:
    
    uint16_t currValue;
    uint16_t prevValue;
    bool filter = false;
    float filterAmount = 0.1;
    bool invert = false;
    
    uint16_t sampleCounter;        // basic counter for DSP

    bool denoising;                // flag to tell handle whether to use the incloming data in the DMA_BUFFER for denoising an idle input signal
    uint16_t idleNoiseThreshold;   // how much noise an idle input signal contains
    uint16_t prevMaxNoiseSample;
    uint16_t prevMinNoiseSample;
};