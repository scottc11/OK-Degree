#include "VoltPerOctave.h"

using namespace DEGREE;

void VoltPerOctave::init()
{
    dac->init();
    setPitchBendRange(5);
}

/**
 * Set the pitch bend range to be applied to 1v/o output
 * NOTE: There are 12 notes, but only 8 possible PB range options, meaning there are preset values for each PB range option via PB_RANGE_MAP global
 * @param value number beyween 0..7
*/
void VoltPerOctave::setPitchBendRange(int value)
{
    if (value < 8)
    {
        pbRangeIndex = value;
        maxPitchBend = dacSemitone * PB_RANGE_MAP[pbRangeIndex]; // multiply semitone DAC value by the max desired number of semitones to be bent
    }
}

void VoltPerOctave::setPitchBend(uint16_t value)
{
    currPitchBend = value;
    this->updateDAC(currNoteIndex, currPitchBend);
}

/**
 * Scale an input value to a number between 0 and maxPitchBend
 * @param input ADC input
 * @param min range floor of input
 * @param max range ceiling of input
*/
uint16_t VoltPerOctave::calculatePitchBend(int input, int min, int max)
{
    return map_num_in_range<uint16_t>(input, min, max, minPitchBend, maxPitchBend);
}

/**
 * @param index index to be mapped to voltage map. ranging 0..DAC_1VO_ARR_SIZE
 * @param pitchBend DAC value corrosponing to the amount of pitch bend to apply to output. positive or negative
*/
void VoltPerOctave::updateDAC(int index, uint16_t pitchBend)
{
    if (index < DAC_1VO_ARR_SIZE)
    {
        currNoteIndex = index;
        currOutput = dacVoltageMap[currNoteIndex] + pitchBend;
        dac->write(dacChannel, currOutput);
    }
}

/**
 * @brief Set the DAC to the lowest possible value this instance will allow (index 0 of voltage map);
 * 
 */
void VoltPerOctave::resetDAC()
{
    dac->write(dacChannel, dacVoltageMap[0]);
}

/**
 * The values in the DAC voltage map are entirely dependant on the op-amp amplification
 * 
 * @brief This function takes pre-set values based on the op-amp gain configuration, and generates an array
 * of 16-bit values which are evenly spaced apart by the 1 v/o equivelent of a semitone
*/
void VoltPerOctave::resetVoltageMap()
{
    float voltPerBit = ((float)V_OUT_MAX - (float)V_OUT_MIN) / (float)DAC_RESOLUTION;
    uint16_t octave = 1.0 / voltPerBit;
    uint16_t floor = (float)CALIBRATION_FLOOR / voltPerBit;
    uint16_t semitone = octave / 12;

    for (int i = 0; i < DAC_1VO_ARR_SIZE; i++)
    {
        dacVoltageMap[i] = floor + (semitone * i);
    }
}