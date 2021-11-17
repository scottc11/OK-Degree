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
#include "Callback.h"
#include "ExpoFilter.h"
#include "DAC8554.h"
#include "ArrayMethods.h"
#include "AnalogHandle.h"

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
    ExpoFilter inputFilter;    //
    ExpoFilter outputFilter;
    Callback<void()> idleCallback;                    // MBED Callback which gets called when the Bender is idle / not-active
    Callback<void(uint16_t bend)> activeCallback;     // MBED Callback which gets called when the Bender is active / being bent
    Callback<void(BendState state)> triStateCallback; // callback which gets called when bender changes from one of three BendState states

    BendState currState;
    BendState prevState;
    int currBend;                                 // 16 bit value (0..65,536)
    float dacOutputRange = BIT_MAX_16 / 2;        // range in which the DAC can output (in either direction)
    int dacOutput;                                // the amount of Control Voltage to apply Pitch Bend DAC
    bool outputInverted;                          // whether to invert the output of the DAC based on how the ADC reads the direction of the bender
    int calibrationSamples[PB_CALIBRATION_RANGE]; // an array which gets populated during initialization phase to determine a debounce value + zeroing

    uint16_t zeroBend;                            // the average ADC value when pitch bend is idle
    uint16_t idleDebounce;                        // for debouncing the ADC when Pitch Bend is idle
    uint16_t maxBend = DEFAULT_MAX_BEND;          // the minimum value the ADC can achieve when Pitch Bend fully pulled
    uint16_t minBend = DEFAULT_MIN_BEND;          // the maximum value the ADC can achieve when Pitch Bend fully pressed

    Bender(DAC8554 *dac_ptr, DAC8554::Channel _dacChan, PinName adcPin, bool inverted = false) : adc(adcPin)
    {
        dac = dac_ptr;
        dacChan = _dacChan;
        outputInverted = inverted;
    };

    void init();
    void poll();
    uint16_t read();
    void calibrateIdle();
    void calibrateMinMax();
    void updateDAC(uint16_t value);
    bool isIdle(uint16_t bend);
    int setMode(int targetMode = 0);
    int calculateOutput(uint16_t value);
    void attachIdleCallback(Callback<void()> func);
    void attachActiveCallback(Callback<void(uint16_t bend)> func);
    void attachTriStateCallback(Callback<void(BendState state)> func);
};