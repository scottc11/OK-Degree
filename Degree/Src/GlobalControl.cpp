#include "GlobalControl.h"

using namespace DEGREE;

uint32_t SETTINGS_BUFFER[SETTINGS_BUFFER_SIZE];

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
    display->clear();

    // Tempo Pot ADC Noise: 1300ish w/ 100nF
    tempoPot.setFilter(0.01);
    okSemaphore *sem_ptr = tempoPot.initDenoising();
    sem_ptr->take(); // wait
    sem_ptr->give();
    tempoPot.log_noise_threshold_to_console("Tempo Pot");
    tempoPot.invertReadings();

    // initialize tempo
    clock->init();
    clock->attachResetCallback(callback(this, &GlobalControl::resetSequencer));
    clock->attachPPQNCallback(callback(this, &GlobalControl::advanceSequencer)); // always do this last
    clock->disableInputCaptureISR(); // pollTempoPot() will re-enable should pot be in teh right position
    currTempoPotValue = tempoPot.read_u16();
    handleTempoAdjustment(currTempoPotValue);
    prevTempoPotValue = currTempoPotValue;
    clock->start();
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
        pollButtons();
        break;
    case CALIBRATING_BENDER:
        for (int i = 0; i < 4; i++)
        {
            uint16_t sample = channels[i]->bender->adc.read_u16();
            channels[i]->bender->adc.sampleMinMax(sample);
        }
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
    if (currTempoPotValue > prevTempoPotValue + 600 || currTempoPotValue < prevTempoPotValue - 600) {
        handleTempoAdjustment(currTempoPotValue);
        prevTempoPotValue = currTempoPotValue;
    }
}

void GlobalControl::handleTempoAdjustment(uint16_t value)
{
    if (value > 1000)
    {
        if (clock->externalInputMode)
        {
            clock->disableInputCaptureISR();
        }
        clock->setPulseFrequency(clock->convertADCReadToTicks(1000, BIT_MAX_16, value));
    }
    else
    {
        // change clock source to input mode by enabling input capture ISR
        clock->enableInputCaptureISR();
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
        break;
    case FREEZE:
        freezeLED.write(HIGH);
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->freeze(true);
        }
        
        break;

    case RESET:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->resetSequence();
        }
        break;

    case Gestures::CALIBRATE_BENDER:
        if (this->mode == CALIBRATING_BENDER)
        {
            this->saveCalibrationDataToFlash();
            display->clear();
            this->mode = DEFAULT;
        }
        else
        {
            this->mode = CALIBRATING_BENDER;
            display->benderCalibration();
            for (int i = 0; i < 4; i++)
            {
                channels[i]->bender->adc.resetMinMax();
            }
            
        }
        break;

    case Gestures::RESET_CALIBRATION_DATA:
        if (gestureFlag)
        {
            this->handleChannelGesture(callback(this, &GlobalControl::resetCalibration1VO));
        } else {
            this->resetCalibrationDataToDefault();
        }
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
            this->enableVCOCalibration(channels[selectedChannel]);
        }
        break;

    case BEND_MODE:
        // iterate over currTouched and setChannelBenderMode if touched
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            if (touchPads->padIsTouched(i, currTouched, prevTouched))
            {
                channels[i]->setBenderMode();
            }
        }
        break;

    case CLEAR_SEQ_TOUCH:
        break;

    case PB_RANGE:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->setUIMode(TouchChannel::UIMode::UI_PITCH_BEND_RANGE);
        }
        break;
    case SEQ_LENGTH:
        this->display->clear();
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            channels[chan]->setUIMode(TouchChannel::UIMode::UI_SEQUENCE_LENGTH);
            channels[chan]->setBenderMode(TouchChannel::BenderMode::BEND_MENU);
        }
        break;
    case RECORD:
        if (!recordEnabled)
        {
            recLED.write(1);
            for (int i = 0; i < CHANNEL_COUNT; i++)
                channels[i]->enableSequenceRecording();
            recordEnabled = true;
        }
        else
        {
            recLED.write(0);
            for (int i = 0; i < CHANNEL_COUNT; i++)
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
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->freeze(false);
        }
        break;
    case RESET:
        break;
    case PB_RANGE:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->setUIMode(TouchChannel::UIMode::UI_DEFAULT);
        }
        break;
    case CLEAR_SEQ_TOUCH:
        if (!gestureFlag)
        {
            for (int i = 0; i < 4; i++)
            {
                channels[i]->sequence.clearAllTouchEvents();
                channels[i]->disableSequenceRecording();
            }
        }
        else // clear only curr touched channels sequences
        {
            channels[getTouchedChannel()]->sequence.clearAllTouchEvents();
            channels[getTouchedChannel()]->disableSequenceRecording();
        }
        break;
    case CLEAR_SEQ_BEND:
        if (!gestureFlag) {
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                channels[i]->sequence.clearAllBendEvents();
                channels[i]->disableSequenceRecording();
            }
        } else {
            channels[getTouchedChannel()]->sequence.clearAllBendEvents();
            channels[getTouchedChannel()]->disableSequenceRecording();
        }
        break;
    case SEQ_LENGTH:
        this->display->clear();
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            if (channels[chan]->sequence.containsEvents())
            {
                display->setSequenceLEDs(chan, channels[chan]->sequence.length, 2, true);
            }
            channels[chan]->setBenderMode(TouchChannel::BenderMode::BEND_OFF);
            channels[chan]->setUIMode(TouchChannel::UIMode::UI_DEFAULT);
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
    Flash flash;
    flash.read(FLASH_CONFIG_ADDR, (uint32_t *)SETTINGS_BUFFER, SETTINGS_BUFFER_SIZE);

    bool dataIsClean = flash.validate(SETTINGS_BUFFER, 4);

    if (dataIsClean)
    {
        // load default 1VO values
        logger_log("\nLoading default values");
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            channels[chan]->output.resetVoltageMap();
            channels[chan]->bender->setMaxBend(DEFAULT_MAX_BEND);
            channels[chan]->bender->setMinBend(DEFAULT_MIN_BEND);
        }
    }
    else
    { // if it does, load the data from flash
        logger_log("\nLoading calibration data from flash");
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            for (int i = 0; i < DAC_1VO_ARR_SIZE; i++)
            {
                int index = this->getCalibrationDataPosition(i, chan);
                channels[chan]->output.dacVoltageMap[i] = (uint16_t)SETTINGS_BUFFER[index];
            }
            channels[chan]->bender->setMinBend(SETTINGS_BUFFER[this->getCalibrationDataPosition(BENDER_MIN_CAL_INDEX, chan)]);
            channels[chan]->bender->setMaxBend(SETTINGS_BUFFER[this->getCalibrationDataPosition(BENDER_MAX_CAL_INDEX, chan)]);
        }
    }
}

/**
 * @brief Save all 4 channels calibration data to flash
 * 
 * NOTE: every time we calibrate a channel, all 4 channels calibration data needs to be re-saved to flash
 * because we have to delete/clear an entire sector of data first.
*/
void GlobalControl::saveCalibrationDataToFlash()
{
    // disable interupts?
    this->display->fill();
    int buffer_position = 0;
    for (int chan = 0; chan < CHANNEL_COUNT; chan++) // channel iterrator
    {
        for (int i = 0; i < DAC_1VO_ARR_SIZE; i++)   // dac array iterrator
        {
            buffer_position = this->getCalibrationDataPosition(i, chan);
            SETTINGS_BUFFER[buffer_position] = channels[chan]->output.dacVoltageMap[i]; // copy values into buffer
        }
        // load max and min Bender calibration data into buffer (two 16bit chars)
        SETTINGS_BUFFER[this->getCalibrationDataPosition(BENDER_MIN_CAL_INDEX, chan)] = channels[chan]->bender->getMinBend();
        SETTINGS_BUFFER[this->getCalibrationDataPosition(BENDER_MAX_CAL_INDEX, chan)] = channels[chan]->bender->getMaxBend();
    }
    // now load this buffer into flash memory
    Flash flash;
    flash.erase(FLASH_CONFIG_ADDR);
    flash.write(FLASH_CONFIG_ADDR, SETTINGS_BUFFER, SETTINGS_BUFFER_SIZE);
    // flash the grid of leds on and off for a sec then exit
    this->display->flash(3, 300);
    this->display->clear();
    logger_log("\nSaved Calibration Data to Flash");
}

void GlobalControl::deleteCalibrationDataFromFlash()
{
    Flash flash;
    flash.erase(FLASH_CONFIG_ADDR);
}

void GlobalControl::resetCalibrationDataToDefault()
{
    this->deleteCalibrationDataFromFlash();
    this->loadCalibrationDataFromFlash();
}

/**
 * @brief reset a channels 1vo calibration data
 * 
 * @param chan index
 */
void GlobalControl::resetCalibration1VO(int chan)
{
    // basically just reset a channels voltage map to default and then save as usual
    channels[chan]->output.resetVoltageMap();
    this->saveCalibrationDataToFlash();
}

/**
 * @brief Calibration data for all channels is stored in a single buffer, this function returns the relative
 * position of a channels calibration data inside that buffer
 *
 * @param data_index
 * @param channel_index
 * @return int
 */
int GlobalControl::getCalibrationDataPosition(int data_index, int channel_index)
{
    return (data_index + CALIBRATION_ARR_SIZE * channel_index);
}

/**
 * Method gets called once every PPQN
 * 
*/
void GlobalControl::advanceSequencer(uint8_t pulse)
{
    if (pulse == 0) {
        tempoLED.write(HIGH);
        tempoGate.write(HIGH);
    } else if (pulse == 4) {
        tempoLED.write(LOW);
        tempoGate.write(LOW);
    }

    for (int i = 0; i < CHANNEL_COUNT; i++)
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
    for (int i = 0; i < CHANNEL_COUNT; i++)
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

void GlobalControl::enableVCOCalibration(TouchChannel *channel)
{
    ctrl_dispatch(CTRL_ACTION::ENTER_1VO_CALIBRATION, channel->channelIndex, 0);
}

void GlobalControl::disableVCOCalibration() {
    this->mode = GlobalControl::Mode::DEFAULT;
}

void GlobalControl::handleChannelGesture(Callback<void(int chan)> callback)
{
    for (int i = 0; i < CHANNEL_COUNT; i++)
    {
        if (touchPads->padIsTouched(i, currTouched, prevTouched))
        {
            callback(i);
            break;
        }
    }
}

/**
 * @brief return the currently touched channel
 * 
 * @return int 
 */
int GlobalControl::getTouchedChannel() {
    int channel = 0;
    for (int i = 0; i < CHANNEL_COUNT; i++)
    {
        if (touchPads->padIsTouched(i, currTouched, prevTouched))
        {
            channel = i;
            break;
        }
    }
    gestureFlag = false;
    return channel;
}