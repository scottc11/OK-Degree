#include "VoltPerOctave.h"

using namespace DEGREE;

bool VoltPerOctave::obtainSample = false;
uint32_t VoltPerOctave::numSamplesTaken = 0;
bool VoltPerOctave::slopeIsPositive = false;
float VoltPerOctave::vcoFrequency = 0;
float VoltPerOctave::freqSamples[MAX_FREQ_SAMPLES] = {};
int VoltPerOctave::freqSampleIndex = 0;
uint16_t VoltPerOctave::currVCOInputVal = 0;
uint16_t VoltPerOctave::prevVCOInputVal = 0;

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
    return scaleIntToRange(input, min, max, minPitchBend, maxPitchBend);
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

/**
 * @brief Calibrate the voltage map based on external frequency detection of VCO
*/
void VoltPerOctave::initCalibration()
{
    
}

void VoltPerOctave::sampleVCO(uint16_t adc_sample)
{   
    // you need to first determine the "range" of the signal going into the ADC, so that you can determine a mid-point
    // for detecting zero crossings

    
    // NEGATIVE SLOPE
    if (adc_sample >= (VCO_ZERO_CROSSING + VCO_ZERO_CROSS_THRESHOLD) && prevVCOInputVal < (VCO_ZERO_CROSSING + VCO_ZERO_CROSS_THRESHOLD) && slopeIsPositive)
    {
        slopeIsPositive = false;
    }
    // POSITIVE SLOPE
    else if (adc_sample <= (VCO_ZERO_CROSSING - VCO_ZERO_CROSS_THRESHOLD) && prevVCOInputVal > (VCO_ZERO_CROSSING - VCO_ZERO_CROSS_THRESHOLD) && !slopeIsPositive)
    {
        float vcoPeriod = numSamplesTaken;           // how many samples have occurred between positive zero crossings
        vcoFrequency = 8000.f / vcoPeriod;           // sample rate divided by period of input signal
        freqSamples[freqSampleIndex] = vcoFrequency; // store sample in array
        numSamplesTaken = 0;                         // reset sample count to zero for the next sampling routine

        if (freqSampleIndex < MAX_FREQ_SAMPLES - 1)
        {
            freqSampleIndex += 1;
        }
        else
        {
            freqSampleIndex = 0;
            obtainSample = false;
        }
        slopeIsPositive = true;
    }

    prevVCOInputVal = adc_sample;
    numSamplesTaken++;
}