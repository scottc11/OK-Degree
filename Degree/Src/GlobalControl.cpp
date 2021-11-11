#include "GlobalControl.h"

using namespace DEGREE;

void GlobalControl::init() {
    this->loadCalibrationDataFromFlash();

    display->init();
    display->clear();

    switches->init();
    switches->attachCallback(callback(this, &GlobalControl::handleSwitchChange));

    buttons->init();
    buttons->setDirection(MCP23017_PORTA, 0xff);
    buttons->setDirection(MCP23017_PORTB, 0xff);
    buttons->setInterupt(MCP23017_PORTA, 0xff);
    buttons->setInterupt(MCP23017_PORTB, 0xff);
    buttons->setPullUp(MCP23017_PORTA, 0xff);
    buttons->setPullUp(MCP23017_PORTB, 0xff);
    buttons->setInputPolarity(MCP23017_PORTA, 0xff);
    buttons->setInputPolarity(MCP23017_PORTB, 0b01111111);
    buttons->digitalReadAB();
    
    touchPads->init();

    channels[0]->init();
    channels[1]->init();
    channels[2]->init();
    channels[3]->init();

    clock->attachPPQNCallback(callback(this, &GlobalControl::advanceSequencer)); // always do this last
}

void GlobalControl::poll()
{
    switches->poll();
    pollButtons();
    pollTouchPads();
    channels[0]->poll();
    channels[1]->poll();
    channels[2]->poll();
    channels[3]->poll();
}

void GlobalControl::handleSwitchChange()
{
    channels[0]->updateDegrees();
    channels[1]->updateDegrees();
    channels[2]->updateDegrees();
    channels[3]->updateDegrees();
}

void GlobalControl::handleButtonInterupt()
{
    buttonInterupt = true;
}

void GlobalControl::handleTouchInterupt() {
    touchDetected = true;
}

void GlobalControl::pollTouchPads() {
    if (touchDetected)
    {
        prevTouched = currTouched;
        currTouched = touchPads->touched() >> 4;
        gestureFlag = true;
        touchDetected = false;
    }
}

/**
 * Poll IO and see if any buttons have been either pressed or released
*/
void GlobalControl::pollButtons()
{
    if (buttonInterupt)
    {
        currButtonsState = buttons->digitalReadAB();
        if (currButtonsState != prevButtonsState)
        {
            for (int i = 0; i < 16; i++)
            {
                // if state went HIGH and was LOW before
                if (bitRead(currButtonsState, i) && !bitRead(prevButtonsState, i))
                {
                    this->handleButtonPress(currButtonsState);
                }
                // if state went LOW and was HIGH before
                if (!bitRead(currButtonsState, i) && bitRead(prevButtonsState, i))
                {
                    this->handleButtonRelease(prevButtonsState);
                }
            }
        }

        // reset polling
        prevButtonsState = currButtonsState;
        buttonInterupt = false;
    }
}

/**
 * Handle Button Press
 * 
*/
void GlobalControl::handleButtonPress(int pad)
{

    switch (pad)
    {
    case CMODE:
        for (int i = 0; i < 4; i++)
        {
            if (touchPads->padIsTouched(i, currTouched, prevTouched))
            {
                channels[i]->toggleMode();
            }
        }
        break;
    case SHIFT:
        // for (int i = 0; i < 4; i++)
        // {
        //     if (touch.padIsTouched(i, currTouched, prevTouched))
        //     {
        //         calibrateChannel(i);
        //     }
        // }
        break;
    case FREEZE:
        // handleFreeze(true);
        break;

    case RESET:
        for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
        {
            channels[i]->resetSequence();
        }
        break;

    case Gestures::CALIBRATE_BENDER:
        // if (this->mode == CALIBRATING_BENDER)
        // {
        //     this->saveCalibrationToFlash();
        //     display->clear();
        //     this->mode = DEFAULT;
        // }
        // else
        // {
        //     this->mode = CALIBRATING_BENDER;
        //     display->benderCalibration();
        // }
        break;

    case Gestures::RESET_CALIBRATION_TO_DEFAULT:
        // display->benderCalibration();
        // saveCalibrationToFlash(true);
        // display->clear();
        break;

    case BEND_MODE:
        // iterate over currTouched and setChannelBenderMode if touched
        for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
        {
            if (touchPads->padIsTouched(i, currTouched, prevTouched))
            {
                channels[i]->setBenderMode();
            }
        }
        break;

    case CLEAR_SEQ:
        break;

    case PB_RANGE:
        // channels[0]->enableUIMode(TouchChannel::PB_RANGE_UI);
        // channels[1]->enableUIMode(TouchChannel::PB_RANGE_UI);
        // channels[2]->enableUIMode(TouchChannel::PB_RANGE_UI);
        // channels[3]->enableUIMode(TouchChannel::PB_RANGE_UI);
        break;
    case SEQ_LENGTH:
        // this->display->clear();
        // for (int chan = 0; chan < 4; chan++)
        // {
        //     channels[chan]->setBenderMode(TouchChannel::BenderMode::BEND_MENU);
        // }
        break;
    case RECORD:
        recLED = !recLED.read();
        // if (!recordEnabled)
        // {
        //     recLED.write(1);
        //     channels[0]->enableSequenceRecording();
        //     channels[1]->enableSequenceRecording();
        //     channels[2]->enableSequenceRecording();
        //     channels[3]->enableSequenceRecording();
        //     recordEnabled = true;
        // }
        // else
        // {
        //     recLED.write(0);
        //     channels[0]->disableSequenceRecording();
        //     channels[1]->disableSequenceRecording();
        //     channels[2]->disableSequenceRecording();
        //     channels[3]->disableSequenceRecording();
        //     recordEnabled = false;
        // }
        break;
    }
}

/**
 * Handle Button Release
*/
void GlobalControl::handleButtonRelease(int pad)
{
    switch (pad)
    {
    case FREEZE:
        // handleFreeze(false);
        break;
    case RESET:
        break;
    case PB_RANGE:
        // channels[0]->disableUIMode();
        // channels[1]->disableUIMode();
        // channels[2]->disableUIMode();
        // channels[3]->disableUIMode();
        break;
    case CLEAR_SEQ:
        // if (!gestureFlag)
        // {
        //     for (int i = 0; i < 4; i++)
        //     {
        //         channels[i]->clearEventSequence();
        //         channels[i]->disableSequenceRecording();
        //     }
        // }
        // gestureFlag = false;
        break;
    case SEQ_LENGTH:
        // this->display->clear();
        // for (int chan = 0; chan < 4; chan++)
        // {
        //     if (channels[chan]->sequenceContainsEvents)
        //     {
        //         display->setSequenceLEDs(chan, channels[chan]->sequence.length, 2, true);
        //     }
        //     channels[chan]->setBenderMode(TouchChannel::BenderMode::BEND_OFF);
        // }
        break;
    case RECORD:
        break;
    }
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
            channels[chan]->output.resetVoltageMap();
        }
    }
    else
    { // if it does, load the data from flash
        for (int chan = 0; chan < 4; chan++)
        {
            for (int i = 0; i < DAC_1VO_ARR_SIZE; i++)
            {
                int index = i + CALIBRATION_ARR_SIZE * chan; // determine falshData index position based on channel
                channels[chan]->output.dacVoltageMap[i] = (uint16_t)buffer[index];
            }
            // channels[chan]->bender.minBend = buffer[BENDER_MIN_CAL_INDEX + CALIBRATION_ARR_SIZE * chan];
            // channels[chan]->bender.maxBend = buffer[BENDER_MAX_CAL_INDEX + CALIBRATION_ARR_SIZE * chan];
        }
    }
}

void GlobalControl::advanceSequencer()
{
    for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
    {
        channels[i]->sequence.advance();
    }
}