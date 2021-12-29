#pragma once

#include "main.h"
#include "DAC8554.h"
#include "Algorithms.h"
#include "PitchFrequencies.h"
#include "AnalogHandle.h"

#ifndef DAC_1VO_ARR_SIZE
#define DAC_1VO_ARR_SIZE 64
#endif

#define DAC_RESOLUTION 65535
#define V_OUT_MIN 0
#define V_OUT_MAX 6.532
#define CALIBRATION_FLOOR 0.2
#define NUM_OCTAVES 6

#define DEFAULT_VOLTAGE_ADJMNT 200
#define MAX_CALIB_ATTEMPTS 20
#define MAX_FREQ_SAMPLES 25          // how many frequency calculations we want to use to obtain our average frequency prediction of the input. The higher the number, the more accurate the result
#define VCO_SAMPLE_RATE_US 125       // 8000hz is equal to 125us (microseconds)
#define VCO_ZERO_CROSSING 60000      // The zero crossing is erelivant as the pre-opamp ADC is not bi-polar. Any value close to the ADC ceiling seems to work
#define VCO_ZERO_CROSS_THRESHOLD 500 // for handling hysterisis at zero crossing point

namespace DEGREE {
    const int PB_RANGE_MAP[8] = {1, 2, 3, 4, 5, 7, 10, 12};
    class VoltPerOctave
    {
    public:
        DAC8554 *dac;                 // pointer to 16 bit DAC driver
        DAC8554::Channel dacChannel; // DAC channel
        AnalogHandle *adc;           // 

        uint16_t currOutput; // value being output to the DAC
        int currNoteIndex;

        uint16_t dacSemitone = 938;               //
        uint16_t dacVoltageMap[DAC_1VO_ARR_SIZE]; // pre/post calibrated 16-bit DAC values

        // PITCH BEND
        int pbRangeIndex = 4; // an index value which gets mapped to PB_RANGE_MAP
        int pbNoteOffset;     // the amount of pitch bend to apply to the 1v/o DAC output. Can be positive/negative centered @ 0

        uint16_t maxPitchBend;     // must be a float!
        uint16_t minPitchBend = 0; // should always be 0
        uint16_t currPitchBend;    // the amount of pitch bend to apply to the 1v/o DAC output. Can be positive/negative centered @ 0

        VoltPerOctave(DAC8554 *_dac, DAC8554::Channel _chan)
        {
            this->dac = _dac;
            this->dacChannel = _chan;
        };

        void init();
        void updateDAC(int index, uint16_t pitchBend);
        void setPitchBendRange(int value);
        void setPitchBend(uint16_t value);
        void bend(uint16_t value);
        uint16_t calculatePitchBend(int input, int min, int max);
        void resetVoltageMap();
        void initCalibration();

        void sampleVCO(uint16_t adc_sample);

        static bool obtainSample;
        static uint32_t numSamplesTaken;
        static bool slopeIsPositive;
        static float vcoFrequency;
        static float freqSamples[MAX_FREQ_SAMPLES];
        static int freqSampleIndex;
        static uint16_t currVCOInputVal;
        static uint16_t prevVCOInputVal;
    };
}