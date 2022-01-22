#include "Bender.h"

void Bender::init()
{
    adc.setFilter(0.1);
    okSemaphore *sem = adc.initDenoising();
    sem->wait();
    adc.log_noise_threshold_to_console("Bender");

    // zero the sensor
    this->zeroBend = adc.avgValueWhenIdle;

    dac->init();
    outputFilter.setAlpha(0.1);
    outputFilter.setInitial(this->dacOutputRange); // set initial value to middle of DAC (0V)
    
    setRatchetThresholds();
    updateDAC(0);
}

/**
Bender A ADC Noise: 568, Idle: 25390
Bender B ADC Noise: 928, Idle: 41385
Bender C ADC Noise: 720, Idle: 30807
Bender D ADC Noise: 760, Idle: 32271
*/
void Bender::calibrateMinMax()
{
    currBend = this->read();
    if (currBend > zeroBend + 1000) // bending upwards
    {
        if (currBend > maxBend)
        {
            maxBend = currBend;
        }
    }
    else if (currBend < zeroBend - 1000) // bending downwards
    {
        if (currBend < minBend)
        {
            minBend = currBend;
        }
    }
}

// polling should no longer check if the bender is idle. It should just update the DAC and call the activeCallback
// there are no cycles being saved either way.
void Bender::poll()
{
    currBend = this->read();

    if (this->isIdle())
    {
        updateDAC(0);
        currState = BEND_IDLE;
        if (idleCallback)
            idleCallback();
    }
    else
    {
        // determine direction of bend
        volatile int myOutput = calculateOutput(currBend);
        updateDAC(myOutput);

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
int Bender::calculateOutput(uint16_t value)
{
    // BEND UP
    if (value > zeroBend && value < maxBend)
    {
        currState = BEND_UP;
        return ((dacOutputRange / (maxBend - zeroBend)) * (value - zeroBend)) * -1; // inverted
    }
    // BEND DOWN
    else if (value < zeroBend && value > minBend)
    {
        currState = BEND_DOWN;
        return ((dacOutputRange / (minBend - zeroBend)) * (value - zeroBend)) * 1; // non-inverted
    }
    // ELSE executes when a bender is poorly calibrated, and exceeds its max or min bend
    else
    {
        return dacOutput; // return whatever the last calulated output was.
    }
}

/**
 * @brief apply a slew filter and write the benders state to the DAC
 * 
 * Output will be between 0V and 2.5V, centered at 2.5V/2
*/
void Bender::updateDAC(uint16_t value)
{
    dacOutput = value; // copy to class member
    dac->write(dacChan, outputFilter(BENDER_ZERO + dacOutput));
}

bool Bender::isIdle()
{
    if (currBend > zeroBend + 500 || currBend < zeroBend - 500)
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
    ratchetThresholds[3] = zeroBend + ((maxBend - zeroBend) / 4);
    ratchetThresholds[2] = zeroBend + ((maxBend - zeroBend) / 3);
    ratchetThresholds[1] = zeroBend + ((maxBend - zeroBend) / 2);
    ratchetThresholds[0] = zeroBend + (maxBend - zeroBend);
    ratchetThresholds[7] = zeroBend - ((zeroBend - minBend) / 4);
    ratchetThresholds[6] = zeroBend - ((zeroBend - minBend) / 3);
    ratchetThresholds[5] = zeroBend - ((zeroBend - minBend) / 2);
    ratchetThresholds[4] = zeroBend - (zeroBend - minBend);
}