#include "GlobalControl.h"

using namespace DEGREE;

uint32_t SETTINGS_BUFFER[SETTINGS_BUFFER_SIZE];

void GlobalControl::init() {
    this->loadCalibrationDataFromFlash();

    display->init();
    display->clear();

    logger_log("\n");
    logger_log("\n** Global Control **");
    logger_log("\nToggle Switches: connected = ");
    logger_log(switches->io->isConnected());
    logger_log(", ISR pin (before) = ");
    logger_log(switches->ioInterupt.read());
    switches->init();
    logger_log(", ISR pin (after) = ");
    logger_log(switches->ioInterupt.read());

    logger_log("\nTactile Buttons: connected = ");
    logger_log(buttons->isConnected());
    logger_log(", ISR pin (before) = ");
    logger_log(ioInterrupt.read());
    buttons->init();
    buttons->setDirection(MCP23017_PORTA, 0xff);
    buttons->setDirection(MCP23017_PORTB, 0xff);
    buttons->setInterupt(MCP23017_PORTA, 0xff);
    buttons->setInterupt(MCP23017_PORTB, 0xff);
    buttons->setPullUp(MCP23017_PORTA, 0xff);
    buttons->setPullUp(MCP23017_PORTB, 0xff);
    buttons->setInputPolarity(MCP23017_PORTA, 0xff);
#if BOARD_VERSION == 41
    buttons->setInputPolarity(MCP23017_PORTB, 0b01111111);
#else
    buttons->setInputPolarity(MCP23017_PORTB, 0b11111111);
#endif
    logger_log(", ISR pin (after) = ");
    logger_log(ioInterrupt.read());

    logger_log("\nTouch Pads:      connected = ");
    logger_log(touchPads->isConnected());
    logger_log(", ISR pin (before) = ");
    logger_log(touchInterrupt.read());
    touchPads->init();
    logger_log(", ISR pin (after) = ");
    logger_log(touchInterrupt.read());

    // Tempo Pot ADC Noise: 1300ish w/ 100nF
    tempoPot.setFilter(0.1);
    okSemaphore *sem_ptr = tempoPot.initDenoising();
    sem_ptr->take(); // wait
    sem_ptr->give();
    tempoPot.log_noise_threshold_to_console("Tempo Pot");
    tempoPot.invertReadings();
    logger_log("\n");

    // initializing channels here might be initializing the SPI while an interrupt is getting fired by
    // the tactile buttons / switches, which may be interrupting this task and using the SPI periph before
    // it is initialized
    channels[0]->init();
    channels[1]->init();
    channels[2]->init();
    channels[3]->init();
    display->clear();

    // initialize tempo
    clock->init();
    clock->attachResetCallback(callback(this, &GlobalControl::resetSequencer));
    clock->attachPPQNCallback(callback(this, &GlobalControl::advanceSequencer)); // always do this last
    clock->disableInputCaptureISR(); // pollTempoPot() will re-enable should pot be in teh right position
    currTempoPotValue = tempoPot.read_u16();
    handleTempoAdjustment(currTempoPotValue);
    prevTempoPotValue = currTempoPotValue;
    clock->start();

    switches->attachCallback(callback(this, &GlobalControl::handleSwitchChange));
    switches->enableInterrupt();
    switches->io->digitalReadAB(); // not ideal, but you have to clear the interrupt after initialization
    ioInterrupt.fall(callback(this, &GlobalControl::handleButtonInterrupt));
    buttons->digitalReadAB();
    touchInterrupt.fall(callback(this, &GlobalControl::handleTouchInterrupt));
}

void GlobalControl::poll()
{
    switch (mode)
    {
    case DEFAULT:
        pollTempoPot();
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
    case HARDWARE_TESTING:
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

void GlobalControl::handleButtonInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t isr_id = ISR_ID_TACTILE_BUTTONS;
    xQueueSendFromISR(qhInterruptQueue, &isr_id, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void GlobalControl::handleTouchInterrupt() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t isr_id = ISR_ID_TOUCH_PADS;
    xQueueSendFromISR(qhInterruptQueue, &isr_id, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void GlobalControl::pollTempoPot()
{
    currTempoPotValue = tempoPot.read_u16();
    if (currTempoPotValue > prevTempoPotValue + 200 || currTempoPotValue < prevTempoPotValue - 200) {
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
        clock->setPulseFrequency(clock->convertADCReadToTicks(TEMPO_POT_MIN_ADC, TEMPO_POT_MAX_ADC, value));
    }
    else
    {
        // change clock source to input mode by enabling input capture ISR
        clock->enableInputCaptureISR();
    }
}

/**
 * @brief
 * | x | x | + | - | D | C | B | A |
 */
void GlobalControl::pollTouchPads() {
    prevTouched = currTouched;

#if BOARD_VERSION == 41 // touch pads do not have + - connected, and are wired differently
    currTouched = touchPads->touched() >> 4;
#else
    currTouched = touchPads->touched();
#endif

    // for (int i = 0; i < CHANNEL_COUNT; i++)
    // {
    //     if (touchPads->padIsTouched(i, currTouched))
    //     {
    //         display->fill(i, 127);
    //         // display->setBlinkStatus(i, true);
    //     }
    //     else
    //     {
    //         display->clear(i);
    //         // display->setBlinkStatus(i, false);
    //     }
    // }

    if (currTouched == 0x00)
    {
        gestureFlag = false;
    }
    else
    {
        gestureFlag = true;
    }
}

/**
 * Poll IO and see if any buttons have been either pressed or released
*/
void GlobalControl::pollButtons()
{
    currButtonsState = buttons->digitalReadAB();
    if (currButtonsState != prevButtonsState)
    {
        for (int i = 0; i < 16; i++)
        {
            // if state went HIGH and was LOW before
            if (bitwise_read_bit(currButtonsState, i) && !bitwise_read_bit(prevButtonsState, i))
            {
                if (mode == Mode::HARDWARE_TESTING)
                {
                    this->handleHardwareTest(currButtonsState);
                }
                else
                {
                    this->handleButtonPress(currButtonsState);
                }
            }
            // if state went LOW and was HIGH before
            if (!bitwise_read_bit(currButtonsState, i) && bitwise_read_bit(prevButtonsState, i))
            {
                if (mode != Mode::HARDWARE_TESTING)
                {
                    this->handleButtonRelease(prevButtonsState);
                }
            }
        }
    }

    // reset polling
    prevButtonsState = currButtonsState;
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
            if (touchPads->padIsTouched(i, currTouched))
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

    case QUANTIZE_SEQ:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            if (channels[i]->sequence.containsTouchEvents)
            {
                channels[i]->sequence.quantize();
            }
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
                if (touchPads->padIsTouched(i, currTouched))
                {
                    selectedChannel = i;
                    break;
                }
            }
            gestureFlag = false;
            this->enableVCOCalibration(channels[selectedChannel]);
        }
        break;
    
    case Gestures::ENTER_HARDWARE_TEST:
        mode = Mode::HARDWARE_TESTING;
        break;

    case Gestures::LOG_SYSTEM_STATUS:
        log_system_status();
        break;

    case BEND_MODE:
        // iterate over currTouched and setChannelBenderMode if touched
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            if (touchPads->padIsTouched(i, currTouched))
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

    case QUANTIZE_AMOUNT:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->setUIMode(TouchChannel::UIMode::UI_QUANTIZE_AMOUNT);
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
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                channels[i]->sequence.clearAllTouchEvents();
                if (!recordEnabled)
                    channels[i]->disableSequenceRecording();
            }
        }
        else // clear only curr touched channels sequences
        {
            channels[getTouchedChannel()]->sequence.clearAllTouchEvents();
            if (!recordEnabled)
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
    case SEQ_LENGTH: // exit sequence length UI
        this->display->clear();
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            if (channels[chan]->sequence.containsEvents())
            {
                display->setSequenceLEDs(chan, channels[chan]->sequence.length, 2, true);
            }
            channels[chan]->setBenderMode((TouchChannel::BenderMode)channels[chan]->prevBenderMode);
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
    this->display->fill(PWM::PWM_MID);
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
    // if (pulse % 16 == 0)
    // {
    //     display_dispatch_isr(DISPLAY_ACTION::PULSE_DISPLAY, CHAN::ALL, 0);
    // }
    

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
        if (touchPads->padIsTouched(i, currTouched))
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
        if (touchPads->padIsTouched(i, currTouched))
        {
            channel = i;
            break;
        }
    }
    gestureFlag = false;
    return channel;
}

void GlobalControl::log_system_status()
{
    logger_log("\nToggle Switches ISR = ");
    logger_log(switches->ioInterupt.read());

    logger_log("\nTactile Buttons ISR pin = ");
    logger_log(ioInterrupt.read());

    logger_log("\nTouch ISR pin = ");
    logger_log(touchInterrupt.read());
}

void GlobalControl::handleHardwareTest(uint16_t pressedButtons)
{
    switch (pressedButtons)
    {
    case HardwareTest::TEST_BEND_OUT_HIGH:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->bender->updateDAC(BIT_MAX_16, true);
        break;
    case HardwareTest::TEST_BEND_OUT_LOW:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->bender->updateDAC(0, true);
        break;
    case HardwareTest::TEST_BEND_OUT_MID:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->bender->updateDAC(BENDER_DAC_ZERO, true);
        break;
    case HardwareTest::TEST_1VO_HIGH:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->output.dac->write(channels[i]->output.dacChannel, BIT_MAX_16);
        break;
    case HardwareTest::TEST_1VO_LOW:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->output.dac->write(channels[i]->output.dacChannel, 0);
        break;
    case HardwareTest::TEST_GATE_HIGH:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->setGate(true);
        break;
    case HardwareTest::TEST_GATE_LOW:
        for (int i = 0; i < CHANNEL_COUNT; i++)
            channels[i]->setGate(false);
        break;
    case HardwareTest::EXIT_HARDWARE_TEST:
        mode = Mode::DEFAULT;
        break;
    default:
        break;
    }
}