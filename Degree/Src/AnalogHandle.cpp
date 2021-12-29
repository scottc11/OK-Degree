#include "AnalogHandle.h"

uint16_t AnalogHandle::tim_prescaler = ADC_TIM_PRESCALER;
uint16_t AnalogHandle::tim_period = ADC_TIM_PERIOD;
bool AnalogHandle::sample_ready = false;

/**
 * @brief read the ADC DMA buffer. Gets converted from 12 bit to 16 bit value
 * 
 * @return uint16_t 16-bit value
 */
uint16_t AnalogHandle::read_u16()
{
    prevValue = currValue;
    currValue = convert12to16(DMA_BUFFER[index]);

    if (filter)
    {
        currValue = (currValue * filterAmount) + (prevValue * (1 - filterAmount));
    }

    return currValue;
}

/**
 * @brief Set the digital filter alpha value for filtering input
 * 
 * @param value 
 */
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
        filterAmount = value;
        filter = true;
    }
}

/**
 * @brief Sample the ADC for the lowest, highest, and median voltage
 * @param duration number of samples to take
*/
uint16_t AnalogHandle::samplePeakToPeak(int numSamples) {
    sampleCounter = 0;
    sample_ready = false;
    while (sampleCounter < numSamples)
    {
        if (sample_ready)
        {
            // sample ADC
            uint16_t sample = this->read_u16();
            if (sample > max)
            {
                max = sample;
            }
            else if (sample < min)
            {
                min = sample;
            }
            sampleCounter++;
            sample_ready = false;
        }
    }
    zeroCrossing = (max - min) / 2;
}

uint16_t AnalogHandle::getZeroCrossing() {
    return zeroCrossing;
}

uint16_t AnalogHandle::getMax() {
    return max;
}

uint16_t AnalogHandle::getMin() {
    return min;
}

/**
 * @brief returns the frequency at which each ADC Channel is getting read.
 * This is equal to TIM3 overflow frquency / number of ADC Channels active
 * 
 * @return uint16_t 
 */
uint16_t AnalogHandle::getSampleFrequency() {
    return (APB2_TIM_FREQ / (tim_prescaler * tim_period)) / ADC_DMA_BUFF_SIZE;
}