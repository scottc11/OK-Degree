#include "GlobalControl.h"

using namespace DEGREE;

void GlobalControl::init() {
    this->loadCalibrationDataFromFlash();

    _degrees->init();

    _channels[0]->init();
    _channels[1]->init();
    _channels[2]->init();
    _channels[3]->init();

    _degrees->attachCallback(callback(this, &GlobalControl::handleDegreeChange));
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

void GlobalControl::loadCalibrationDataFromFlash()
{
    volatile uint32_t buffer[CALIBRATION_ARR_SIZE * 4];
    Flash flash;
    flash.read(FLASH_CONFIG_ADDR, (uint32_t *)buffer, CALIBRATION_ARR_SIZE * 4);

    // check if calibration data exists
    if (buffer[0] == 0xFF && buffer[1] == 0xFF) // if it doesn't, load default 1vo values
    {
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
                _channels[chan]->_output.dacVoltageMap[i] = buffer[index];
            }
            // _channels[chan]->bender.minBend = buffer[BENDER_MIN_CAL_INDEX + CALIBRATION_ARR_SIZE * chan];
            // _channels[chan]->bender.maxBend = buffer[BENDER_MAX_CAL_INDEX + CALIBRATION_ARR_SIZE * chan];
        }
    }
}