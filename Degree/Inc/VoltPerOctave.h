#pragma once

#include "main.h"
#include "logger.h"
#include "DAC8554.h"
#include "Algorithms.h"
#include "PitchFrequencies.h"
#include "AnalogHandle.h"
#include "MultiChanADC.h"

#ifndef DAC_1VO_ARR_SIZE
#define DAC_1VO_ARR_SIZE 64
#endif

#define DAC_RESOLUTION 65535
#define V_OUT_MIN 0
#define V_OUT_MAX 6.532f
#define CALIBRATION_FLOOR 0.2f
#define NUM_OCTAVES 6

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

        VoltPerOctave(DAC8554 *_dac, DAC8554::Channel _chan, AnalogHandle *_adc)
        {
            this->dac = _dac;
            this->dacChannel = _chan;
            this->adc = _adc;
        };

        void init();
        void updateDAC(int index, uint16_t pitchBend);
        void setPitchBendRange(int value);
        void setPitchBend(uint16_t value);
        void bend(uint16_t value);
        uint16_t calculatePitchBend(int input, int min, int max);
        void resetVoltageMap();
    };
}