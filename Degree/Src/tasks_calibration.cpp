#include "tasks_calibration.h"

using namespace DEGREE;

static uint16_t signalZeroCrossing = 0;            // The zero crossing is erelivant as the pre-opamp ADC is not bi-polar. Any value close to the ADC ceiling seems to work
static uint32_t numSamplesTaken = 0;               // how many samples have elapsed between zero crossing events
static bool slopeIsPositive = false;               // which direction the signals voltage is moving towards (false = towards GND, true = towards VDD)
static float freqSamples[MAX_FREQ_SAMPLES] = {};   // array to store frequency samples for averaging
static uint16_t prev_adc_sample = 0;

SemaphoreHandle_t sem_obtain_freq;
SemaphoreHandle_t sem_calibrate;

static xTaskHandle thStartCalibration;
static xTaskHandle thExitCalibration;
static xTaskHandle thCalibrate;

/**
 * @brief Takes approx. 78 of stack space while running
 *
 * @param params
 */
void taskObtainSignalFrequency(void *params)
{
    TouchChannel *channel = (TouchChannel *)params;
    
    thStartCalibration = xTaskGetCurrentTaskHandle();

    uint16_t sample;
    float vcoFrequency = 0;  // the calculated frequency sample
    int freqSampleIndex = 0; // index for storing new frequency sample into freqSamples array

    channel->display->clear();
    channel->display->drawSpiral(channel->channelIndex, true, 50);
    channel->display->clear();
    
    channel->adc.disableFilter(); // must not filter ADC input

    multi_chan_adc_set_sample_rate(&hadc1, &htim3, CALIBRATION_SAMPLE_RATE_HZ); // set ADC timer overflow frequency to 16000hz (twice the freq of B8)

    // sample peak to peak;
    okSemaphore *sem = channel->adc.beginMinMaxSampling(2000); // sampling time should be longer than the lowest possible note frequency
    sem->wait();
    channel->adc.log_min_max("CV");
    signalZeroCrossing = channel->adc.getInputMedian();

    sem_obtain_freq = xSemaphoreCreateBinary();
    sem_calibrate = xSemaphoreCreateBinary();

    xTaskCreate(taskCalibrate, "calibrate", RTOS_STACK_SIZE_MIN, channel, RTOS_PRIORITY_MED, &thCalibrate);

    xSemaphoreGive(sem_obtain_freq);

    logger_log_task_watermark();

    while (1)
    {
        xSemaphoreTake(sem_obtain_freq, portMAX_DELAY);
        xQueueReceive(channel->adc.queue.handle, &sample, portMAX_DELAY);
        
        uint16_t curr_adc_sample = channel->adc.read_u16(); // obtain the sample from DMA buffer
        
        // NEGATIVE SLOPE
        if (curr_adc_sample >= (signalZeroCrossing + ZERO_CROSS_THRESHOLD) && prev_adc_sample < (signalZeroCrossing + ZERO_CROSS_THRESHOLD) && slopeIsPositive)
        {
            slopeIsPositive = false;
        }
        // POSITIVE SLOPE
        else if (curr_adc_sample <= (signalZeroCrossing - ZERO_CROSS_THRESHOLD) && prev_adc_sample > (signalZeroCrossing - ZERO_CROSS_THRESHOLD) && !slopeIsPositive)
        {
            float vcoPeriod = numSamplesTaken;                                                // how many samples have occurred between positive zero crossings
            vcoFrequency = (float)multi_chan_adc_get_sample_rate(&hadc1, &htim3) / vcoPeriod; // sample rate divided by period of input signal
            freqSamples[freqSampleIndex] = vcoFrequency;                                      // store sample in array
            numSamplesTaken = 0;                                                              // reset sample count to zero for the next sampling routine

            // NOTE: you could eliminate the freqSamples array by first, ignoring the first iteration of sampling, and then just adding
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

        prev_adc_sample = curr_adc_sample;
        numSamplesTaken++;
        xSemaphoreGive(sem_obtain_freq);
    }
}

/**
 * @brief Once an accurate frequency has been detected, this task executes and adjusts the DAC output of a channel till the incoming frequency matches a target frequency
 * 
 * @param params 
 */
void taskCalibrate(void *params)
{
    TouchChannel *channel = (TouchChannel *)params;

    uint16_t dacAdjustment = DEFAULT_VOLTAGE_ADJMNT;
    int calibrationAttemps = 0; // for limiting the number of calibration attempts
    float currAvgFreq;
    float prevAvgFreq;
    uint16_t newDacValue;
    float targetFreq;           // where we want the frequency to get to
    int iteration = 0;          // the current interation in the voltage map array
    int initialPitchIndex;      // the index of the initial target frequency in PITCH_FREQ_ARR

    newDacValue = channel->output.dacVoltageMap[iteration]; // start at the lowest possible dac voltage
    channel->output.dac->write(channel->output.dacChannel, channel->output.dacVoltageMap[iteration]); // start at zero

    while (1)
    {
        xSemaphoreTake(sem_calibrate, portMAX_DELAY);
        currAvgFreq = calculateAverageFreq();
        
        // handle first iteration of calibrating by finding the frequency in PITCH_FREQ_ARR closest to the currently sampled frequency
        if (iteration == 0)
        {
            initialPitchIndex = arr_find_closest_float(const_cast<float *>(PITCH_FREQ_ARR), NUM_PITCH_FREQENCIES, currAvgFreq);
            logger_log("\nInitial Target Frequency: ");
            logger_log(PITCH_FREQ_ARR[initialPitchIndex]);

            if (initialPitchIndex + DAC_1VO_ARR_SIZE > NUM_PITCH_FREQENCIES) // if the starting PITCH_FREQ_ARR index is too high, you will overshoot the array. Must notify the UI to lower VCO input frequency
            {
                logger_log("\nVCO Input Frequency Too High. ");
                logger_log("INDEX: ");
                logger_log(initialPitchIndex);
                logger_log("\nMust be less than -> ");
                logger_log(NUM_PITCH_FREQENCIES - DAC_1VO_ARR_SIZE);
                // what is the maximum starting target frequency?
            }
            
        }

        // obtain the target frequency
        targetFreq = PITCH_FREQ_ARR[initialPitchIndex + iteration];        

        // if currAvgFreq is close enough to targetFreq, or max cal attempts has been reached, break out of while loop and move to the next 'note'
        if ((currAvgFreq <= targetFreq + TUNING_TOLERANCE && currAvgFreq >= targetFreq - TUNING_TOLERANCE) || calibrationAttemps > MAX_CALIB_ATTEMPTS)
        {
            channel->output.dacVoltageMap[iteration] = newDacValue > BIT_MAX_16 ? BIT_MAX_16 : newDacValue; // replace current DAC value with adjusted value (NOTE: must cap value or else it will roll over to zero)
            logger_log("\nIteration ");
            logger_log(iteration);
            logger_log(" DONE, attempts Taken: ");
            logger_log(calibrationAttemps);

            int ledIndex = map_num_in_range<int>(iteration, 0, DAC_1VO_ARR_SIZE, 0, 15);
            channel->display->setChannelLED(channel->channelIndex, ledIndex, true);

            // if we are on the final iteration, then some how breakout of all this crap.
            if (iteration == DAC_1VO_ARR_SIZE - 1) {
                logger_log("\n\n*** CALIBRATION FINISHED ***");
                channel->display->setChannelLED(channel->channelIndex, 15, true);
                // send a notification to exitCalibration task
                xTaskNotify(thExitCalibration, 0, eNotifyAction::eNoAction);
            }

            calibrationAttemps = 0;
            dacAdjustment = DEFAULT_VOLTAGE_ADJMNT;
            iteration++;
            newDacValue = channel->output.dacVoltageMap[iteration]; // prepare for the next iteration
            vTaskDelay(CALIBRATION_DAC_SETTLE_TIME); // wait for DAC to settle
            xSemaphoreGive(sem_obtain_freq);
        }
        else
        {
            // every time currAvgFreq over/undershoots the desired frequency, decrement the 'dacAdjustment' value by half.
            if (currAvgFreq > targetFreq + TUNING_TOLERANCE)                 // overshot target freq
            {
                if (prevAvgFreq < targetFreq - TUNING_TOLERANCE)
                {
                    dacAdjustment = (dacAdjustment / 2) + 1; // + 1 so it never becomes zero
                }
                newDacValue -= dacAdjustment;
            }

            else if (currAvgFreq < targetFreq - TUNING_TOLERANCE) // undershot target freq
            {
                if (prevAvgFreq > targetFreq + TUNING_TOLERANCE)
                {
                    dacAdjustment = (dacAdjustment / 2) + 1; // so it never becomes zero
                }
                newDacValue += dacAdjustment;
            }
            prevAvgFreq = currAvgFreq;
            calibrationAttemps++;
            channel->output.dac->write(channel->output.dacChannel, newDacValue);
            vTaskDelay(CALIBRATION_DAC_SETTLE_TIME); // wait for DAC to settle
            xSemaphoreGive(sem_obtain_freq);
        }
        
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

/**
 * @brief 
 * 
 * @param params 
 */
void taskExitCalibration(void *params)
{
    GlobalControl *control = (GlobalControl *)params;
    thExitCalibration = xTaskGetCurrentTaskHandle();
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // flash the grid of leds on and off for a sec then exit
        control->display->flash(3, 300);
        control->display->clear();

        vTaskDelete(thCalibrate);
        vTaskDelete(thStartCalibration);
        multi_chan_adc_set_sample_rate(&hadc1, &htim3, ADC_SAMPLE_RATE_HZ);
        control->disableVCOCalibration();
    }
    
}

// void taskCalibrationHandler(void *params) {
//     while (1)
//     {
//         // take freq_sample_ready semaphore
//         switch (notification)
//         {
//         case TUNE_VCO:
//             // give tuning semaphore
//             break;
//         case CALIBRATE_VCO:
//             // give calibrate semaphore
//             break;
//         default:
//             break;
//         }
//     }
// }