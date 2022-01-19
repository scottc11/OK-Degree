#include "AnalogHandle.h"

SemaphoreHandle_t AnalogHandle::semaphore;
AnalogHandle *AnalogHandle::_instances[ADC_DMA_BUFF_SIZE] = {0};

void AnalogHandle::calculateSignalNoise(uint16_t sample)
{
    // get max read, get min read, get avg read
    if (sampleCounter < ADC_SAMPLE_COUNTER_LIMIT)
    {
        if (sample > prevMaxNoiseSample)
        {
            prevMaxNoiseSample = sample;
        }
        else if (sample < prevMinNoiseSample)
        {
            prevMinNoiseSample = sample;
        }
        sampleCounter++;
    }
    else
    {
        denoising = false;
        sampleCounter = 0; // reset back to 0 for later use
        idleNoiseThreshold = (prevMaxNoiseSample - prevMinNoiseSample) / 2;
    }
}

void AnalogHandle::sampleReadyCallback(uint16_t sample)
{
    prevValue = currValue;
    currValue = convert12to16(sample);
    if (this->denoising) {
        this->calculateSignalNoise(currValue);
    }
    if (filter) {
        currValue = (currValue * filterAmount) + (prevValue * (1 - filterAmount));
    }
    // set value
}

void AnalogHandle::RouteConversionCompleteCallback() // static
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(AnalogHandle::semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void AnalogHandle::sampleReadyTask(void *params) {
    while (1)
    {
        xSemaphoreTake(AnalogHandle::semaphore, portMAX_DELAY);
        for (auto ins : _instances)
        {
            if (ins) // if instance not NULL
            {
                ins->sampleReadyCallback(AnalogHandle::DMA_BUFFER[ins->index]);
            }
        }
    }
}