#include "AnalogHandle.h"

SemaphoreHandle_t AnalogHandle::semaphore;
AnalogHandle *AnalogHandle::_instances[ADC_DMA_BUFF_SIZE] = {0};

AnalogHandle::AnalogHandle(PinName pin)
{
    // iterate over static member ADC_PINS and match index to pin
    for (int i = 0; i < ADC_DMA_BUFF_SIZE; i++)
    {
        if (pin == ADC_PINS[i])
        {
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

uint16_t AnalogHandle::read_u16()
{
    return invert ? BIT_MAX_16 - currValue : currValue;
}

void AnalogHandle::invertReadings()
{
    this->invert = !this->invert;
}

void AnalogHandle::setFilter(float value)
{
    if (value == 0)
    {
        filter = false;
    }
    else if (value > (float)1)
    {
        // raise error
        filter = false;
    }
    else
    {
        prevValue = convert12to16(DMA_BUFFER[index]);
        filterAmount = value;
        filter = true;
    }
}

/**
 * @brief takes the denoising semaphore and gives it once calculation is finished.
 * NOTE: don't forget to "give()" the semaphore back after waiting for it.
 * 
 * @return okSemaphore* 
 */
okSemaphore* AnalogHandle::initDenoising() {
    this->samplingNoise = true;
    denoisingSemaphore.take(); // create a semaphore
    return &denoisingSemaphore;
}

// set this as a task so that in the main loop you block with a semaphore until this task gives the semaphore back (once it has completed)
void AnalogHandle::sampleSignalNoise(uint16_t sample)
{
    // get max read, get min read, get avg read
    if (sampleCounter < ADC_SAMPLE_COUNTER_LIMIT)
    {
        if (sampleCounter == 0) {
            noiseCeiling = sample;
            noiseFloor = sample;
        } else {
            if (sample > noiseCeiling)
            {
                noiseCeiling = sample;
            }
            else if (sample < noiseFloor)
            {
                noiseFloor = sample;
            }
        }
        sampleCounter++;
    }
    else
    {
        samplingNoise = false;
        sampleCounter = 0; // reset back to 0 for later use
        idleNoiseThreshold = (noiseCeiling - noiseFloor) / 2;
        avgValueWhenIdle = noiseFloor + idleNoiseThreshold;
        denoisingSemaphore.give();
    }
}

/**
 * @brief This is not exactly a callback, its really a "sender" task
 * 
 * @param sample 
 */
void AnalogHandle::sampleReadyCallback(uint16_t sample)
{
    prevValue = currValue;
    currValue = convert12to16(sample);
    if (filter) {
        currValue = filter_one_pole<uint16_t>(currValue, prevValue, filterAmount);
    }
    if (samplingNoise)
    {
        this->sampleSignalNoise(currValue);
        if (samplingProgressCallback) samplingProgressCallback(sampleCounter);
    }
    if (this->samplingMinMax) // execute if task has a semaphore?
    {
        if (sampleCounter < sampleTime) {
            this->sampleMinMax(currValue);
            sampleCounter++;
        } else {
            samplingMinMax = false;
            sampleCounter = 0;
            // maybe calulate median here
            sampleSemaphore.give();
        }
        
    }
}


/**
 * @brief A static member function which gets called as an ISR whenever the ADC DMA Conversion is completed
 * 
 */
void AnalogHandle::RouteConversionCompleteCallback() // static
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(AnalogHandle::semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief This task waits for a semaphore to be made available by the RouteConversionCompleteCallback()
 * When it takes the semaphore, the task will trigger a callback function for every AnalogHandle 
 * instance that exists.
 *
 * @param params
 */
void AnalogHandle::sampleReadyTask(void *params) {
    logger_log_task_watermark();
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

void AnalogHandle::log_noise_threshold_to_console(char const *source_id)
{
    logger_log("\n");
    logger_log(source_id);
    logger_log(" ADC Noise: ");
    logger_log(this->idleNoiseThreshold);
    logger_log(", Idle: ");
    logger_log(this->avgValueWhenIdle);
}

void AnalogHandle::log_min_max(char const *source_id) {
    logger_log(source_id);
    logger_log(" Signal Max: ");
    logger_log(this->inputMax);
    logger_log(" || Signal Min: ");
    logger_log(this->inputMin);
    logger_log("\n");
}

/**
 * @brief resets the min and max input values to the currently read value on the ADC Pin
 */
void AnalogHandle::resetMinMax()
{
    this->setInputMax(currValue + 1);
    this->setInputMin(currValue - 1);
}

void AnalogHandle::setInputMax(uint16_t value)
{
    this->inputMax = value;
}

void AnalogHandle::setInputMin(uint16_t value)
{
    this->inputMin = value;
}

/**
 * @brief 
 * 
 * @param numSamples how many samples ie. how long to sample for.
 * @return okSemaphore* 
 */
okSemaphore * AnalogHandle::beginMinMaxSampling(uint16_t numSamples) {
    this->resetMinMax();
    this->sampleCounter = 0;
    this->sampleTime = numSamples;
    this->samplingMinMax = true;
    this->sampleSemaphore.take();
    return &sampleSemaphore;
}

/**
 * @brief determine a signals min and max values
 */
void AnalogHandle::sampleMinMax(uint16_t sample)
{
    if (sample > inputMax)
    {
        this->setInputMax(sample);
    }
    else if (sample < inputMin)
    {
        this->setInputMin(sample);
    }
}

void AnalogHandle::attachSamplingProgressCallback(Callback<void(uint16_t progress)> func)
{
    samplingProgressCallback = func;
}

void AnalogHandle::detachSamplingProgressCallback()
{
    samplingProgressCallback = NULL;
}