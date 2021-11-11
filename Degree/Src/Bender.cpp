#include "Bender.h"

void Bender::init()
{
    dac->init();
    outputFilter.setAlpha(0.05);
    outputFilter.setInitial(this->dacOutputRange); // set initial value to middle of DAC (0V)
    calibrateIdle();
    updateDAC(0);
}

void Bender::calibrateIdle()
{
    // must set initial value for digital filter
    inputFilter.setInitial(adc.read_u16());

    // populate calibration array
    for (int i = 0; i < PB_CALIBRATION_RANGE; i++)
    {
        calibrationSamples[i] = this->read();
        HAL_Delay(1);
    }

    // find min/max value from calibration results
    int max = arr_max(calibrationSamples, PB_CALIBRATION_RANGE);
    int min = arr_min(calibrationSamples, PB_CALIBRATION_RANGE);
    this->idleDebounce = (max - min) + 25;

    // zero the sensor
    this->zeroBend = arr_average(calibrationSamples, PB_CALIBRATION_RANGE);
    return;
}

/**
 * chan A @ 220ohm gain: Max = 45211, Min = 27228, zero = 35432, debounce = 496
 * chan B @ 220ohm gain: Max = XXXXX, Min = XXXXX, zero = 32931, debounce = 337
 * chan C @ 220ohm gain: Max = 48276, Min = 28566, zero = 38468, debounce = 448
 * chan D @ 220ohm gain: Max = 43002, Min = 25974, zero = 35851, debounce = 320
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
    dac->write(dacChan, outputFilter(PB_ZERO + dacOutput));
}

bool Bender::isIdle()
{
    if (currBend > zeroBend + idleDebounce || currBend < zeroBend - idleDebounce)
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