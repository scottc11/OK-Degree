#include "GlobalControl.h"

using namespace DEGREE;

void GlobalControl::init() {
    this->loadCalibrationDataFromFlash();

    serial.init();
    serial.transmit("initialized");

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

    clock->attachResetCallback(callback(this, &GlobalControl::resetSequencer));
    clock->attachPPQNCallback(callback(this, &GlobalControl::advanceSequencer)); // always do this last
}

void GlobalControl::poll()
{
    switch (mode)
    {
    case DEFAULT:
        pollTempoPot();
        switches->poll();
        pollButtons();
        pollTouchPads();
        channels[0]->poll();
        channels[1]->poll();
        channels[2]->poll();
        channels[3]->poll();
        break;
    case CALIBRATING_1VO:
        // dooo
        pollButtons();
        break;
    default:
        break;
    }
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

void GlobalControl::pollTempoPot()
{
    currTempoPotValue = tempoPot.read_u16();
    if (currTempoPotValue > prevTempoPotValue + 50 || currTempoPotValue < prevTempoPotValue - 50) {
        clock->setFrequency(currTempoPotValue);
        prevTempoPotValue = currTempoPotValue;
    }
    
}

void GlobalControl::pollTouchPads() {
    if (touchDetected)
    {
        prevTouched = currTouched;
        currTouched = touchPads->touched() >> 4;
        if (currTouched == 0x00) {
            gestureFlag = false;
        } else {
            gestureFlag = true;
        }
        
        touchDetected = false;
    }
}

/**
 * Poll IO and see if any buttons have been either pressed or released
*/
void GlobalControl::pollButtons()
{
    if (ioInterrupt.read() == 0)
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
    buttonInterupt = false;
}

/**
 * Handle Button Press
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
        freezeLED.write(HIGH);
        for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
        {
            channels[i]->freeze(true);
        }
        
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

    case Gestures::CALIBRATE_1VO:
        if (gestureFlag) {
            for (int i = 0; i < 4; i++)
            {
                if (touchPads->padIsTouched(i, currTouched, prevTouched))
                {
                    selectedChannel = i;
                    break;
                }
            }
            gestureFlag = false;
            channels[selectedChannel]->enableCalibration();
            this->mode = CALIBRATING_1VO;
        } else {
            channels[selectedChannel]->disableCalibration();
            this->mode = DEFAULT;
        }
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
        this->display->clear();
        for (int chan = 0; chan < NUM_DEGREE_CHANNELS; chan++)
        {
            channels[chan]->setBenderMode(TouchChannel::BenderMode::BEND_MENU);
        }
        break;
    case RECORD:
        if (!recordEnabled)
        {
            recLED.write(1);
            for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
                channels[i]->enableSequenceRecording();
            recordEnabled = true;
        }
        else
        {
            recLED.write(0);
            for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
                channels[i]->disableSequenceRecording();
            recordEnabled = false;
        }
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
        freezeLED.write(LOW);
        for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
        {
            channels[i]->freeze(false);
        }
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
        if (!gestureFlag)
        {
            for (int i = 0; i < 4; i++)
            {
                channels[i]->sequence.clear();
                channels[i]->disableSequenceRecording();
            }
        }
        else // clear only curr touched channels sequences
        {
            for (int i = 0; i < 4; i++)
            {
                if (touchPads->padIsTouched(i, currTouched, prevTouched))
                {
                    channels[i]->sequence.clear();
                    channels[i]->disableSequenceRecording();
                }
            }
            gestureFlag = false;
        }

        break;
    case SEQ_LENGTH:
        this->display->clear();
        for (int chan = 0; chan < NUM_DEGREE_CHANNELS; chan++)
        {
            if (channels[chan]->sequence.containsEvents)
            {
                display->setSequenceLEDs(chan, channels[chan]->sequence.length, 2, true);
            }
            channels[chan]->setBenderMode(TouchChannel::BenderMode::BEND_OFF);
        }
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

/**
 * Method gets called once every PPQN
 * 
*/
void GlobalControl::advanceSequencer(uint8_t pulse)
{    
    serial.transmit(pulse);
    serial.transmit("\n");

    // channels[0]->setGate(pulse % 2);
    if (pulse == 0) {
        tempoLED.write(HIGH);
        tempoGate.write(HIGH);
    } else if (pulse == 4) {
        tempoLED.write(LOW);
        tempoGate.write(LOW);
    }

    for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
    {
        channels[i]->sequence.advance();
        channels[i]->setTickerFlag();
    }

#ifdef CLOCK_DEBUG
    if (pulse == PPQN - 2) {
        tempoGate.write(HIGH);
    }

    if (pulse == PPQN - 1)
    {
        tempoGate.write(LOW);
    }
#endif
}

/**
 * @brief reset all sequencers PPQN position to 0
 * 
 * The issue might be that the sequence is not always running too slow, but running to fast. In other words, the
 * pulse count overtakes the external clocks rising edge. To avoid this scenario, you need to halt further execution of the sequence should it reach PPQN - 1 
 * prior to the ext clock rising edge. Thing is, the sequence would first need to know that it is being externally clocked...
 * 
 * TODO: external clock is ignored unless the CLOCK knob is set to its minimum value and tap tempo is reset.
 * 
 * Currently, your clock division is very off. The higher the ext clock signal is, you lose an increasing amount of pulses.
 * Ex. @ 120bpm sequence will reset on pulse 84 (ie. missing 12 PPQNs)
 * Ex. @ 132bpm sequence missing 20 PPQNs
 */
void GlobalControl::resetSequencer()
{
    for (int i = 0; i < NUM_DEGREE_CHANNELS; i++)
    {
        // try just setting the sequence to 0, set any potential gates low. You may miss a note but 🤷‍♂️

        // if sequence is not on its final PPQN of its step, then trigger all remaining PPQNs in current step until currPPQN == 0
        if (channels[i]->sequence.currStepPosition != 0) {
            while (channels[i]->sequence.currStepPosition != 0)
            {
                // you can't be calling i2c / spi functions here, meaning you can't execute unexecuted sequence events until you first abstract
                // the DAC and LED components of an event out into a seperate thread.

                // incrementing the clock will at least keep the sequence in sync with an external clock
                channels[i]->sequence.advance();
            }
            channels[i]->setTickerFlag();
        }
    }
}