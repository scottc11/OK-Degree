#include "tasks_calibration.h"

using namespace DEGREE;

static uint16_t signalZeroCrossing = 0;            // The zero crossing is erelivant as the pre-opamp ADC is not bi-polar. Any value close to the ADC ceiling seems to work
static uint32_t numSamplesTaken = 0;               // how many samples have elapsed between zero crossing events
static bool slopeIsPositive = false;               // which direction the signals voltage is moving towards (false = towards GND, true = towards VDD)
static float vcoFrequency = 0;                     // the calculated frequency sample
static float freqSamples[MAX_FREQ_SAMPLES] = {};   // array to store frequency samples for averaging
static int freqSampleIndex = 0;                    // index for storing new frequency sample into freqSamples array
static uint16_t currVCOInputVal = 0;
static uint16_t prevVCOInputVal = 0;

SemaphoreHandle_t sem_obtain_freq;
SemaphoreHandle_t sem_calibrate;

/**
 * @brief Takes approx. 78 of stack space while running
 *
 * @param params
 */
void taskObtainSignalFrequency(void *params)
{
    TouchChannel *chan = (TouchChannel *)params;
    uint16_t sample;
    chan->display->clear();
    chan->display->drawSquare(chan->channelIndex);
    chan->adc.disableFilter();

    multi_chan_adc_set_sample_rate(&hadc1, &htim3, 16000); // set ADC timer overflow frequency to 16000hz (twice the freq of B8)

    // sample peak to peak;
    okSemaphore *sem = chan->adc.beginMinMaxSampling(2000); // sampling time should be longer than the lowest possible note frequency
    sem->wait();
    chan->adc.log_min_max("CV");
    signalZeroCrossing = chan->adc.getInputMedian();

    sem_obtain_freq = xSemaphoreCreateBinary();
    sem_calibrate = xSemaphoreCreateBinary();

    xTaskCreate(taskCalibrate, "calibrate", RTOS_STACK_SIZE_MIN, NULL, 3, NULL);

    xSemaphoreGive(sem_obtain_freq);

    logger_log_task_watermark();

    while (1)
    {
        xSemaphoreTake(sem_obtain_freq, portMAX_DELAY);
        xQueueReceive(chan->adc.queue.handle, &sample, portMAX_DELAY);
        
        uint16_t adc_sample = chan->adc.read_u16(); // obtain the sample from DMA buffer
        
        // NEGATIVE SLOPE
        if (adc_sample >= (signalZeroCrossing + ZERO_CROSS_THRESHOLD) && prevVCOInputVal < (signalZeroCrossing + ZERO_CROSS_THRESHOLD) && slopeIsPositive)
        {
            slopeIsPositive = false;
        }
        // POSITIVE SLOPE
        else if (adc_sample <= (signalZeroCrossing - ZERO_CROSS_THRESHOLD) && prevVCOInputVal > (signalZeroCrossing - ZERO_CROSS_THRESHOLD) && !slopeIsPositive)
        {
            float vcoPeriod = numSamplesTaken;                                                // how many samples have occurred between positive zero crossings
            vcoFrequency = (float)multi_chan_adc_get_sample_rate(&hadc1, &htim3) / vcoPeriod; // sample rate divided by period of input signal
            freqSamples[freqSampleIndex] = vcoFrequency;                                      // store sample in array
            numSamplesTaken = 0;                                                              // reset sample count to zero for the next sampling routine

            // NOTE: you could illiminate the freqSamples array by first, ignoring the first iteration of sampling, and then just adding
            // each newly sampled frequency to a sum. This way you could increase the MAX_FREQ_SAMPLES as high as you want without requiring the
            // initialization of a massive array to hold all the samples.
            if (freqSampleIndex < MAX_FREQ_SAMPLES - 1)
            {
                // logger_log("\n");
                // logger_log(vcoFrequency);
                freqSampleIndex += 1;
            }
            else
            {
                xSemaphoreGive(sem_calibrate); // give semaphore to calibrate task
                xSemaphoreTake(sem_obtain_freq, portMAX_DELAY); // wait till other tasks gives this semaphore back, then obtain next notes freq.
                freqSampleIndex = 0;
            }
            slopeIsPositive = true;
        }

        prevVCOInputVal = adc_sample;
        numSamplesTaken++;
        xSemaphoreGive(sem_obtain_freq);
    }
}

void taskCalibrate(void *params)
{
    while (1)
    {
        xSemaphoreTake(sem_calibrate, portMAX_DELAY);
        logger_log("\n");
        logger_log("avg: ");
        logger_log(calculateAverageFreq());
        xSemaphoreGive(sem_obtain_freq);
    }
    
}

/**
 * @brief Calculate Average frequency
 * NOTE: skipping the first sample is hard to explain but its important...
 * @return float
 */
float calculateAverageFreq()
{
    float sum = 0;
    for (int i = 1; i < MAX_FREQ_SAMPLES; i++)
    {
        sum += freqSamples[i];
    }
    return (float)(sum / (MAX_FREQ_SAMPLES - 1));
}