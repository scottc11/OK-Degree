#include "GlobalControl.h"

using namespace DEGREE;

uint32_t SETTINGS_BUFFER[SETTINGS_BUFFER_SIZE];

void GlobalControl::init() {
    suspend_sequencer_task();
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
    clock->attachStepCallback(callback(this, &GlobalControl::handleStepCallback));
    clock->attachResetCallback(callback(this, &GlobalControl::resetSequencer));
    clock->attachBarResetCallback(callback(this, &GlobalControl::handleBarReset));
    clock->attachPPQNCallback(callback(this, &GlobalControl::advanceSequencer)); // always do this last
    clock->disableInputCaptureISR(); // pollTempoPot() will re-enable should pot be in teh right position
    currTempoPotValue = tempoPot.read_u16();
    handleTempoAdjustment(currTempoPotValue);
    prevTempoPotValue = currTempoPotValue;
    clock->start();

    switches->attachCallback(callback(this, &GlobalControl::handleSwitchChange));
    switches->enableInterrupt();
    switches->updateDegreeStates(); // not ideal, but you have to clear the interrupt after initialization
    ioInterrupt.fall(callback(this, &GlobalControl::handleButtonInterrupt));
    buttons->digitalReadAB();
    touchInterrupt.fall(callback(this, &GlobalControl::handleTouchInterrupt));
    resume_sequencer_task();
}

void GlobalControl::poll()
{
    switch (mode)
    {
    case DEFAULT:
        pollTempoPot(); // TODO: possibly move this into sequence handler on every clock tick
        break;
    case VCO_CALIBRATION:
        break;
    case CALIBRATING_BENDER:
        for (int i = 0; i < 4; i++)
        {
            uint16_t sample = channels[i]->bender->adc.read_u16();
            channels[i]->bender->adc.sampleMinMax(sample);
        }
        break;
    case HARDWARE_TESTING:
        break;
    default:
        break;
    }
}

/**
 * @brief exit out of current mode
 */
void GlobalControl::exitCurrentMode()
{
    switch (mode)
    {
    case ControlMode::DEFAULT:
        /* code */
        break;
    case ControlMode::VCO_CALIBRATION:
        actionTimer.stop();
        actionTimer.detachCallback();
        display->disableBlink();
        display->clear();
        resume_sequencer_task();
        dispatch_sequencer_event(CHAN::ALL, SEQ::DISPLAY, 0);
        // enter default mode, check if any sequences are active and running, if so, update the display
        break;
    case ControlMode::CALIBRATING_BENDER:
        /* code */
        break;

    case ControlMode::SETTING_SEQUENCE_LENGTH:
        // iterate over each channel and then keep the display LEDs on or off based on sequence containing events
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            if (!channels[chan]->sequence.containsEvents())
            {
                display->clear(chan);
            }
            channels[chan]->disableBenderOverride();
            channels[chan]->setUIMode(TouchChannel::UIMode::UI_PLAYBACK);
            display->disableBlink();
        }
        break;

    case ControlMode::SETTING_QUANTIZE_AMOUNT:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->setUIMode(TouchChannel::UIMode::UI_PLAYBACK);
        }
        break;

    case ControlMode::HARDWARE_TESTING:
        /* code */
        break;
    }
    // always revert to default mode
    mode = ControlMode::DEFAULT;
}

void GlobalControl::handleSwitchChange()
{
    dispatch_sequencer_event(CHAN::ALL, SEQ::HANDLE_DEGREE, 0);
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

    if (currTouched == 0x00) {
        gestureFlag = false;
    } else {
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
                if (mode == ControlMode::HARDWARE_TESTING)
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
                if (mode != ControlMode::HARDWARE_TESTING)
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
    // let any butten press break out of currently running "mode"
    if (actionExitFlag == ACTION_EXIT_STAGE_1)
    {
        actionExitFlag = ACTION_EXIT_STAGE_2;
        return;
    }
    switch (pad)
    {
    case CMODE:
        for (int i = 0; i < 4; i++)
        {
            if (touchPads->padIsTouched(i, currTouched))
            {
                dispatch_sequencer_event(CHAN(i), SEQ::TOGGLE_MODE, 0);
            }
        }
        break;
    case SHIFT:
        break;
    case FREEZE:
        freezeLED.write(HIGH);
        this->handleFreeze(true);
        break;

    case RESET:
        if (gestureFlag)
        {
            for (int i = 0; i < CHANNEL_COUNT; i++) {
                if (touchPads->padIsTouched(i, currTouched))
                    dispatch_sequencer_event(CHAN(i), SEQ::RESET, 0);
            }
        } else {
            dispatch_sequencer_event(CHAN::ALL, SEQ::RESET, 0);
        }
        break;

    case QUANTIZE_SEQ:
        dispatch_sequencer_event(CHAN::ALL, SEQ::QUANTIZE, 0);
        break;

    case Gestures::CALIBRATE_BENDER:
        if (this->mode == CALIBRATING_BENDER)
        {
            this->saveCalibrationDataToFlash();
            display->disableBlink();
            this->mode = DEFAULT;
            logger_log("\nEXIT Bender Calibration");
            resume_sequencer_task();
        }
        else
        {
            suspend_sequencer_task();
            logger_log("\nENTER Bender Calibration");
            this->mode = CALIBRATING_BENDER;
            display->enableBlink();
            display->benderCalibration();
            for (int i = 0; i < 4; i++)
            {
                channels[i]->bender->adc.resetMinMax();
            }
        }
        break;

    case Gestures::SETTINGS_RESET:
        if (gestureFlag)
        {
            this->handleChannelGesture(callback(this, &GlobalControl::resetCalibration1VO));
        } else {
            this->resetCalibrationDataToDefault();
        }
        break;

    case Gestures::SETTINGS_SAVE:
        this->saveCalibrationDataToFlash();
        break;

    case Gestures::CALIBRATE_1VO:
        actionExitFlag = ACTION_EXIT_STAGE_1; // set exit flag
        mode = ControlMode::VCO_CALIBRATION;
        suspend_sequencer_task();
        display->enableBlink();
        display->fill(30, true);
        actionCounter = 0; // reset
        actionCounterLimit = 15;
        actionTimer.attachCallback(callback(this, &GlobalControl::pressHold), 100, true);
        actionTimer.start();
        break;
    
    case Gestures::ENTER_HARDWARE_TEST:
        mode = ControlMode::HARDWARE_TESTING;
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
        actionExitFlag = ACTION_EXIT_STAGE_1;
        mode = ControlMode::SETTING_QUANTIZE_AMOUNT;
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->setUIMode(TouchChannel::UIMode::UI_QUANTIZE_AMOUNT);
        }
        break;

    case SEQ_LENGTH:
        actionExitFlag = ACTION_EXIT_STAGE_1;
        mode = ControlMode::SETTING_SEQUENCE_LENGTH;
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            // deactivate any downstream channel interactions with the display,
            channels[chan]->setUIMode(TouchChannel::UIMode::UI_SEQUENCE_LENGTH);
            
            // take control of all the benders
            channels[chan]->enableBenderOverride();
            
            // draw the current clock time signature to the display
            drawTimeSignatureToDisplay();

            // bender tristate callback will send commands to the rtos
        }
        break;

    case RECORD:
        if (!recordEnabled)
        {
            recLED.write(1);
            dispatch_sequencer_event(CHAN::ALL, SEQ::RECORD_ARM, 0);
            recordEnabled = true;
        }
        else
        {
            dispatch_sequencer_event(CHAN::ALL, SEQ::RECORD_DISARM, 0);
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
    if (actionExitFlag == ACTION_EXIT_STAGE_2)
    {
        exitCurrentMode();
        actionExitFlag = ACTION_EXIT_CLEAR;
        return;
    }
    switch (pad)
    {
    case FREEZE:
        freezeLED.write(LOW);
        handleFreeze(false);
        break;
    case RESET:
        break;
    case PB_RANGE:
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            channels[i]->setUIMode(TouchChannel::UIMode::UI_PLAYBACK);
        }
        break;
        
    case CLEAR_SEQ_TOUCH:
        if (!gestureFlag)
        {
            dispatch_sequencer_event(CHAN::ALL, SEQ::CLEAR_TOUCH, 0);
            if (!recordEnabled)
                dispatch_sequencer_event(CHAN::ALL, SEQ::RECORD_DISABLE, 0); // why do you even have to do this?
        }
        else // clear only curr touched channels sequences
        {
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                if (touchPads->padIsTouched(i, currTouched))
                {
                    dispatch_sequencer_event((CHAN)i, SEQ::CLEAR_TOUCH, 0);
                    if (!recordEnabled) // this chunk is a shortcut for clearing the display
                        dispatch_sequencer_event((CHAN)i, SEQ::RECORD_DISABLE, 0);
                }
            }
            gestureFlag = false;
        }
        break;

    case CLEAR_SEQ_BEND:
        if (!gestureFlag) {
            dispatch_sequencer_event(CHAN::ALL, SEQ::CLEAR_BEND, 0);
            if (!recordEnabled)
                dispatch_sequencer_event(CHAN::ALL, SEQ::RECORD_DISABLE, 0); // why do you even have to do this?
        } else {
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                if (touchPads->padIsTouched(i, currTouched))
                {
                    dispatch_sequencer_event((CHAN)i, SEQ::CLEAR_BEND, 0);
                    if (!recordEnabled)
                        dispatch_sequencer_event((CHAN)i, SEQ::RECORD_DISABLE, 0);
                }
            }
            gestureFlag = false;
        }
        break;
    case SEQ_LENGTH:
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
        logger_log("\nChannel Settings Source: DEFAULT");
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            channels[chan]->output.resetVoltageMap();
        }
    }
    else
    { // if it does, load the data from flash
        logger_log("\nChannel Settings Source: FLASH");
        for (int chan = 0; chan < CHANNEL_COUNT; chan++)
        {
            for (int i = SETTINGS_DAC_1VO; i < DAC_1VO_ARR_SIZE; i++)
            {
                channels[chan]->output.dacVoltageMap[i] = (uint16_t)getSettingsBufferValue(i, chan);
            }
            channels[chan]->bender->setMinBend(getSettingsBufferValue(SETTINGS_BENDER_MIN, chan));
            channels[chan]->bender->setMaxBend(getSettingsBufferValue(SETTINGS_BENDER_MAX, chan));
            channels[chan]->currBenderMode = getSettingsBufferValue(SETTINGS_BENDER_MODE, chan);
            channels[chan]->output.setPitchBendRange(getSettingsBufferValue(SETTINGS_PITCH_BEND_RANGE, chan));
            channels[chan]->sequence.setQuantizeAmount(static_cast<QUANT>(getSettingsBufferValue(SETTINGS_QUANTIZE_AMOUNT, chan)));
            channels[chan]->sequence.setLength(getSettingsBufferValue(SETTINGS_SEQ_LENGTH, chan));
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
    this->display->fill(PWM::PWM_MID, true);
    int buffer_position = 0;
    for (int chan = 0; chan < CHANNEL_COUNT; chan++) // channel iterrator
    {
        for (int i = SETTINGS_DAC_1VO; i < DAC_1VO_ARR_SIZE; i++) // dac array iterrator
        {
            setSettingsBufferValue(i, chan, channels[chan]->output.dacVoltageMap[i]);
        }
        // load max and min Bender calibration data into buffer (two 16bit chars)
        setSettingsBufferValue(SETTINGS_BENDER_MIN, chan, channels[chan]->bender->adc.getInputMin());
        setSettingsBufferValue(SETTINGS_BENDER_MAX, chan, channels[chan]->bender->adc.getInputMax());
        setSettingsBufferValue(SETTINGS_BENDER_MODE, chan, channels[chan]->currBenderMode);
        setSettingsBufferValue(SETTINGS_PITCH_BEND_RANGE, chan, channels[chan]->output.pbRangeIndex);
        setSettingsBufferValue(SETTINGS_QUANTIZE_AMOUNT, chan, (uint16_t)channels[chan]->sequence.quantizeAmount);
        setSettingsBufferValue(SETTINGS_SEQ_LENGTH, chan, channels[chan]->sequence.length);
    }
    // now load this buffer into flash memory
    Flash flash;
    flash.erase(FLASH_CONFIG_ADDR); // should flash.erase be moved into flash.write method?
    flash.write(FLASH_CONFIG_ADDR, SETTINGS_BUFFER, SETTINGS_BUFFER_SIZE);
    // flash the grid of leds on and off for a sec then exit
    this->display->flash(3, 300);
    this->display->clear();
    logger_log("\nSaved Calibration Data to Flash");
}

void GlobalControl::deleteCalibrationDataFromFlash()
{
    display->setScene(SCENE::SETTINGS);
    display->resetScene();
    display->enableBlink();
    display->fill(127, true);

    Flash flash;
    flash.erase(FLASH_CONFIG_ADDR);

    for (int i = 0; i < DISPLAY_COLUMN_COUNT; i++)
    {
        int led = DISPLAY_COLUMN_COUNT - 1 - i; // go backwards
        display->setColumn(led, 30, true);
        vTaskDelay(50);
    }
    display->setScene(SCENE::SEQUENCER);
    display->redrawScene();
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
int GlobalControl::calculatePositionInSettingsBuffer(int data_index, int channel_index)
{
    return (data_index + CALIBRATION_ARR_SIZE * channel_index);
}

/**
 * @brief Calibration data for all channels is stored in a single buffer, this function returns the value
 * at the given position of that buffer
 *
 * @param data_index
 * @param channel_index
 * @return int
 */
int GlobalControl::getSettingsBufferValue(int position, int channel)
{
    position = calculatePositionInSettingsBuffer(position, channel);
    return (int)SETTINGS_BUFFER[position];
}

void GlobalControl::setSettingsBufferValue(int position, int channel, int data)
{
    position = calculatePositionInSettingsBuffer(position, channel);
    SETTINGS_BUFFER[position] = data;
}

/**
 * @brief Called within ISR, advances all channels sequence by 1, and handles global clock output
 * 
 * @param pulse 
 */
void GlobalControl::advanceSequencer(uint8_t pulse)
{
    if (pulse == 0) {
        tempoLED.write(HIGH);
        tempoGate.write(HIGH);
        if (clock->step == 0) {
            freezeLED.write(HIGH);
        }
    } else if (pulse == 4) {
        tempoLED.write(LOW);
        tempoGate.write(LOW);
        if (freezeBtn == false)
        {
            freezeLED.write(LOW);
        }
    }

    dispatch_sequencer_event_ISR(CHAN::ALL, SEQ::ADVANCE, pulse);
}

/**
 * @brief reset all sequencers PPQN position to 0
 * 
 * The issue might be that the sequence is not always running too slow, but running to fast. In other words, the
 * pulse count overtakes the external clocks rising edge. To avoid this scenario, you need to halt further execution of the sequence should it reach PPQN - 1 
 * prior to the ext clock rising edge. Thing is, the sequence would first need to know that it is being externally clocked...
 * 
 * Currently, your clock division is very off. The higher the ext clock signal is, you lose an increasing amount of pulses.
 * Ex. @ 120bpm sequence will reset on pulse 84 (ie. missing 12 PPQNs)
 * Ex. @ 132bpm sequence missing 20 PPQNs
 */
void GlobalControl::resetSequencer(uint8_t pulse)
{
    dispatch_sequencer_event_ISR(CHAN::ALL, SEQ::CORRECT, 0);
}

/**
 * @brief triggers in ISR everytime clock progresses by one step
 * 
 * @param step 
 */
void GlobalControl::handleStepCallback(uint16_t step)
{
    dispatch_sequencer_event_ISR(CHAN::ALL, SEQ::QUARTER_NOTE_OVERFLOW, 0);
}

/**
 * @brief draw the current clock time signature to the display
 *
 */
void GlobalControl::drawTimeSignatureToDisplay()
{
    display->clear();
    for (int i = 0; i < clock->stepsPerBar; i++)
    {
        display->setLED(i, 127, false);
    }
}


/**
 * @brief function that triggers at the begining of every bar
 * 
 */
void GlobalControl::handleBarReset()
{
    dispatch_sequencer_event_ISR(CHAN::ALL, SEQ::BAR_RESET, 0);
}

void GlobalControl::handleFreeze(bool freeze) {
    // set freeze variable here, and check in clock callback if freeze is true/false
    freezeBtn = freeze;
    if (this->gestureFlag)
    {
        for (int i = 0; i < CHANNEL_COUNT; i++)
        {
            if (touchPads->padIsTouched(i, currTouched))
                dispatch_sequencer_event(CHAN(i), SEQ::FREEZE, freeze);
        }
    }
    else
    {
        dispatch_sequencer_event(CHAN::ALL, SEQ::FREEZE, freeze);
    }
}

void GlobalControl::disableVCOCalibration() {
    this->mode = ControlMode::DEFAULT;
}

/**
 * @brief auto-reload software timer callback function which steps through the currently selected channels 16 display LEDs
 * and makes them brighter. Once function increments its counter to 16, enter that channel into VCO calibration
 */
void GlobalControl::pressHold() {
    if (gestureFlag)
    {
        int touchedChannel = getTouchedChannel();
        display->setSpiralLED(touchedChannel, actionCounter, 255, false);
        actionCounter++;
        if (actionCounter > actionCounterLimit) {
            actionTimer.stop();
            selectedChannel = touchedChannel; // this is kinda meh
            ctrl_dispatch(CTRL_ACTION::ENTER_1VO_CALIBRATION, touchedChannel, 0);
        }
    } else {
        // reset
        if (actionCounter != 0)
        {
            display->fill(30, true);
        }
        actionCounter = 0;
    }
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
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        channels[i]->logPeripherals();
        channels[i]->output.logVoltageMap();
    }
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
        mode = ControlMode::DEFAULT;
        break;
    default:
        break;
    }
}