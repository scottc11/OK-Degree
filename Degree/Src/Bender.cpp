/**
Bender A ADC Noise: 568, Idle: 25390
Bender B ADC Noise: 928, Idle: 41385
Bender C ADC Noise: 720, Idle: 30807
Bender D ADC Noise: 760, Idle: 32271
*/

#include "Bender.h"

void Bender::init()
{
    adc.setFilter(0.1);
    okSemaphore *sem = adc.initDenoising();
    sem->wait();
    adc.log_noise_threshold_to_console("Bender");

    dac->init();
    
    setRatchetThresholds();
    updateDAC(BENDER_ZERO);
}

// polling should no longer check if the bender is idle. It should just update the DAC and call the activeCallback
// there are no cycles being saved either way.
void Bender::poll()
{
    currBend = this->read();

    if (this->isIdle(currBend))
    {
        updateDAC(BENDER_ZERO);
        currState = BEND_IDLE;
        if (idleCallback)
            idleCallback();
    }
    else
    {
        // determine direction of bend
        calculateOutput(currBend);

        if (activeCallback) {
            activeCallback(currBend);
        }
            
    }

    // handle tri-state
    if (currState != prevState)
    {
        if (triStateCallback)
        {
            triStateCallback(currState);
        }
        prevState = currState;
    }
}

/**
 * Map the ADC input to a greater range so the DAC can make use of all 16-bits
 * 
 * Two formulas are needed because the zero to max range and the min to zero range are usually different
*/
uint16_t Bender::calculateOutput(uint16_t value)
{
    // BEND UP
    uint16_t val;
    if (value > this->getIdleValue() && value < this->getMaxBend())
    {
        currState = BEND_UP;
        //     inverted
        val = (((float)dacOutputRange / (this->getMaxBend() - this->getIdleValue())) * (value - this->getIdleValue()));
        this->updateDAC(BENDER_ZERO - val);
        return val;
    }
    // BEND DOWN
    else if (value < this->getIdleValue() && value > this->getMinBend())
    {
        currState = BEND_DOWN;
        //      non-inverted
        val = (((float)dacOutputRange / (this->getMinBend() - this->getIdleValue())) * (value - this->getIdleValue()));
        this->updateDAC(BENDER_ZERO + val);
        return val;
    }
    // ELSE executes when a bender is poorly calibrated, and exceeds its max or min bend
    else
    {
        return currOutput; // return whatever the last calulated output was.
    }
}

/**
 * @brief apply a slew filter and write the benders state to the DAC
 * 
 * Output will be between 0V and 2.5V, centered at 2.5V/2
*/
void Bender::updateDAC(uint16_t value)
{
    prevOutput = currOutput;
    currOutput = filter_one_pole<uint16_t>(value, prevOutput, 0.065);
    dac->write(dacChan, currOutput);
}

bool Bender::isIdle(uint16_t bend)
{
    if (bend > zeroBend + idleDebounce || bend < zeroBend - idleDebounce)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void Bender::attachIdleCallback(Callback<void()> func)
{
    idleCallback = func;
}

void Bender::attachActiveCallback(Callback<void(uint16_t bend)> func)
{
    activeCallback = func;
}

void Bender::attachTriStateCallback(Callback<void(BendState state)> func)
{
    triStateCallback = func;
}

uint16_t Bender::read()
{
    return adc.read_u16();
}

void Bender::setRatchetThresholds()
{    
    ratchetThresholds[3] = this->getIdleValue() + ((this->getMaxBend() - this->getIdleValue()) / 4);
    ratchetThresholds[2] = this->getIdleValue() + ((this->getMaxBend() - this->getIdleValue()) / 3);
    ratchetThresholds[1] = this->getIdleValue() + ((this->getMaxBend() - this->getIdleValue()) / 2);
    ratchetThresholds[0] = this->getIdleValue() + (this->getMaxBend() - this->getIdleValue());
    ratchetThresholds[7] = this->getIdleValue() - ((this->getIdleValue() - this->getMinBend()) / 4);
    ratchetThresholds[6] = this->getIdleValue() - ((this->getIdleValue() - this->getMinBend()) / 3);
    ratchetThresholds[5] = this->getIdleValue() - ((this->getIdleValue() - this->getMinBend()) / 2);
    ratchetThresholds[4] = this->getIdleValue() - (this->getIdleValue() - this->getMinBend());
}

/**
 * @brief the average ADC value when bender is idle / not being touched
 *
 * @return uint16_t
 */
uint16_t Bender::getIdleValue() {
    return adc.avgValueWhenIdle;
}

/**
 * @brief // the maximum value the ADC can achieve when Pitch Bend fully pressed
 *
 * @return uint16_t
 */
uint16_t Bender::getMaxBend()
{
    return adc.inputMax;
}

/**
 * @brief the minimum value the ADC can achieve when Pitch Bend fully pulled
 *
 * @return uint16_t
 */
uint16_t Bender::getMinBend()
{
    return adc.inputMin;
}