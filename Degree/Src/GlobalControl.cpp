#include "GlobalControl.h"

using namespace DEGREE;

void GlobalControl::init() {
    this->loadCalibrationDataFromFlash();

    _degrees->init();
    _degrees->attachCallback(callback(this, &GlobalControl::handleDegreeChange));

    _channels[0]->init();
    _channels[1]->init();
    _channels[2]->init();
    _channels[3]->init();
}

void GlobalControl::poll()
{
    _degrees->poll();
    _channels[0]->poll();
    _channels[1]->poll();
    _channels[2]->poll();
    _channels[3]->poll();
}

void GlobalControl::handleDegreeChange()
{
    _channels[0]->updateDegrees();
    _channels[1]->updateDegrees();
    _channels[2]->updateDegrees();
    _channels[3]->updateDegrees();
}

/**
 * NOTE: Careful with the creation of this buffer, as it is quite large and may push the memory past its limits
*/ 
void GlobalControl::loadCalibrationDataFromFlash()
{
    uint32_t buffer[CALIBRATION_ARR_SIZE * 4];
    Flash flash;
    flash.read(FLASH_CONFIG_ADDR, (uint32_t *)buffer, CALIBRATION_ARR_SIZE * 4);

    // check if calibration data exists by testing contents of buffer
    uint32_t count = 0;
    for (int i = 0; i < 4; i++)
        count += (uint16_t)buffer[i];

    if (count == 4 * 0xFFFF) // if all data is equal to 0xFFFF, than no calibration exists
    {
        // load default 1VO values
        for (int chan = 0; chan < 4; chan++)
        {
            _channels[chan]->_output.resetVoltageMap();
        }
    }
    else
    { // if it does, load the data from flash
        for (int chan = 0; chan < 4; chan++)
        {
            for (int i = 0; i < DAC_1VO_ARR_SIZE; i++)
            {
                int index = i + CALIBRATION_ARR_SIZE * chan; // determine falshData index position based on channel
                _channels[chan]->_output.dacVoltageMap[i] = (uint16_t)buffer[index];
            }
            // _channels[chan]->bender.minBend = buffer[BENDER_MIN_CAL_INDEX + CALIBRATION_ARR_SIZE * chan];
            // _channels[chan]->bender.maxBend = buffer[BENDER_MAX_CAL_INDEX + CALIBRATION_ARR_SIZE * chan];
        }
    }
}