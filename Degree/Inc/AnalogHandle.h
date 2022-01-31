#pragma once

#include "main.h"
#include "filters.h"
#include "okSemaphore.h"
#include "okQueue.h"
#include "logger.h"

#define ADC_SAMPLE_COUNTER_LIMIT 2000
#define ADC_DEFAULT_INPUT_MAX BIT_MAX_16
#define ADC_DEFAULT_INPUT_MIN 0

/**
 * @brief Simple class that pulls the data from a DMA buffer into an object
*/ 
class AnalogHandle {
public:
    AnalogHandle(PinName pin);

    int index;

    okQueue<uint16_t, 1> queue;
    okSemaphore denoisingSemaphore;
    okSemaphore sampleSemaphore;
    uint16_t idleNoiseThreshold;               // how much noise an idle input signal contains
    uint16_t avgValueWhenIdle;                 // where the sensor sits when "idle" (only relevant for sensors)
    uint16_t noiseCeiling;                     // highest read noise value when idle
    uint16_t noiseFloor;                       // lowest read noise value when idle
    uint16_t inputMax = ADC_DEFAULT_INPUT_MAX; // highest read value from signal
    uint16_t inputMin = ADC_DEFAULT_INPUT_MIN; // lowest read value from signal

    uint16_t read_u16();
    void setFilter(float value);
    void enableFilter() { filter = true; }
    void disableFilter() { filter = false; }
    void invertReadings();
    
    void log_noise_threshold_to_console(char const *source_id);
    void log_min_max(char const *source_id);

    okSemaphore * initDenoising();
    void calculateSignalNoise(uint16_t sample);

    okSemaphore * beginMinMaxSampling(uint16_t numSamples);
    void resetMinMax();
    void sampleMinMax(uint16_t sample);
    void setInputMax(uint16_t value);
    void setInputMin(uint16_t value);
    uint16_t getInputMax(void) { return inputMax; }
    uint16_t getInputMin(void) { return inputMin; }
    uint16_t getInputMedian(void) { return inputMin + ((inputMax - inputMin) / 2); }

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

    uint16_t sampleTime;      // how long / how many samples you wish to sample for
    uint16_t sampleCounter;   // basic counter for DSP
    bool samplingNoise;       // flag to tell handle whether to use the incloming data in the DMA_BUFFER for denoising an idle input signal
    bool samplingMinMax;
};