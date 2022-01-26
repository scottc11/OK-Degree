/**
 * Each bender sensor has a unique polarity, regardless of which channel they are plugged in to.
 * 
 * CHAN A -> Up is down, down is up
 * CHAN B -> Up is up, Down is down
 * CHAN C -> Up is up, Down is down
 * CHAN D -> Up is down, down is up
*/

#pragma once

#include "main.h"
#include "okSemaphore.h"
#include "Callback.h"
#include "DAC8554.h"
#include "ArrayMethods.h"
#include "AnalogHandle.h"
#include "filters.h"

#define PB_CALIBRATION_RANGE 300
#define DEFAULT_MAX_BEND 50000
#define DEFAULT_MIN_BEND 15000

class Bender
{
public:
    enum BendState
    {
        BEND_UP,
        BEND_DOWN,
        BEND_IDLE
    };

    DAC8554 *dac;              // pointer to Pitch Bends DAC
    DAC8554::Channel dacChan;  // which dac channel to address
    AnalogHandle adc;              // CV input via Instrumentation Amplifier
    Callback<void()> idleCallback;                    // MBED Callback which gets called when the Bender is idle / not-active
    Callback<void(uint16_t bend)> activeCallback;     // MBED Callback which gets called when the Bender is active / being bent
    Callback<void(BendState state)> triStateCallback; // callback which gets called when bender changes from one of three BendState states

    BendState currState;
    BendState prevState;
    int currBend;                                 // 16 bit value (0..65,536)
    uint16_t dacOutputRange = BIT_MAX_16 / 2;     // range in which the DAC can output (in either direction)
    uint16_t currOutput;                          // current DAC output
    uint16_t prevOutput;                          // previous DAC output
    bool outputInverted;                          // whether to invert the output of the DAC based on how the ADC reads the direction of the bender
    int calibrationSamples[PB_CALIBRATION_RANGE]; // an array which gets populated during initialization phase to determine a debounce value + zeroing
    uint16_t ratchetThresholds[8];

    Bender(DAC8554 *dac_ptr, DAC8554::Channel _dacChan, PinName adcPin, bool inverted = false) : adc(adcPin)
    {
        dac = dac_ptr;
        dacChan = _dacChan;
        outputInverted = inverted;
    };

    void init();
    void poll();
    uint16_t read();
    uint16_t getIdleValue();
    uint16_t getMaxBend();
    uint16_t getMinBend();
    void setMaxBend(uint16_t value) { adc.setInputMax(value); }
    void setMinBend(uint16_t value) { adc.setInputMin(value); }

    void setRatchetThresholds();
    void updateDAC(uint16_t value);
    bool isIdle();
    int setMode(int targetMode = 0);
    uint16_t calculateOutput(uint16_t value);
    void attachIdleCallback(Callback<void()> func);
    void attachActiveCallback(Callback<void(uint16_t bend)> func);
    void attachTriStateCallback(Callback<void(BendState state)> func);
};