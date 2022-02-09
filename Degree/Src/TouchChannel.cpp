#include "TouchChannel.h"

using namespace DEGREE;

void TouchChannel::init()
{
    xTaskCreate(TouchChannel::taskHandleTouch, "handleTouch", RTOS_STACK_SIZE_MIN, this, RTOS_PRIORITY_HIGH, &this->handleTouchTaskHandle);

    output.init(); // must init this first (for the dac)
    
    adc.setFilter(0.1);

    sequence.init();

    bender->init();
    bender->attachActiveCallback(callback(this, &TouchChannel::benderActiveCallback));
    bender->attachIdleCallback(callback(this, &TouchChannel::benderIdleCallback));
    bender->attachTriStateCallback(callback(this, &TouchChannel::benderTriStateCallback));

    // initialize channel touch pads
    touchPads->init();
    touchPads->attachInterruptCallback(callback(this, &TouchChannel::handleTouchInterrupt));
    touchPads->attachCallbackTouched(callback(this, &TouchChannel::onTouch));
    touchPads->attachCallbackReleased(callback(this, &TouchChannel::onRelease));
    touchPads->enable();

    // initialize LED Driver
    _leds->init();
    _leds->setBlinkFrequency(SX1509::FAST);

    for (int i = 0; i < 16; i++) {
        _leds->ledConfig(i);
        setLED(i, OFF);
    }

    setMode(MONO);
    setBenderMode(PITCH_BEND);
}

/** ------------------------------------------------------------------------
 *         POLL    POLL    POLL    POLL    POLL    POLL    POLL    POLL    
 *  ------------------------------------------------------------------------
*/
void TouchChannel::poll()
{
    if (!freezeChannel)
    {
        if (tickerFlag)
        {

            bender->poll();

            if (currMode == QUANTIZER || currMode == QUANTIZER_LOOP)
            {
                handleCVInput();
            }

            if (currMode == MONO_LOOP || currMode == QUANTIZER_LOOP)
            {
                handleSequence(sequence.currPosition);
            }

            clearTickerFlag();
        }
    }
}

/**
 * @brief set the channels mode
*/ 
void TouchChannel::setMode(TouchChannelMode targetMode)
{
    prevMode = currMode;
    currMode = targetMode;

    // start from a clean slate by setting all the LEDs LOW
    for (int i = 0; i < DEGREE_COUNT; i++) {
        setDegreeLed(i, DIM_MED);
        setDegreeLed(i, OFF);
    }
    setLED(CHANNEL_REC_LED, OFF);
    setLED(CHANNEL_QUANT_LED, OFF);
    sequence.playbackEnabled = false;

    switch (currMode)
    {
    case MONO:
        triggerNote(currDegree, currOctave, SUSTAIN);
        updateOctaveLeds(currOctave);
        break;
    case MONO_LOOP:
        sequence.playbackEnabled = true;
        setLED(CHANNEL_REC_LED, ON);
        triggerNote(currDegree, currOctave, SUSTAIN);
        break;
    case QUANTIZER:
        setLED(CHANNEL_QUANT_LED, ON);
        setActiveDegrees(activeDegrees);
        triggerNote(currDegree, currOctave, NOTE_OFF);
        break;
    case QUANTIZER_LOOP:
        sequence.playbackEnabled = true;
        setLED(CHANNEL_REC_LED, ON);
        setActiveDegrees(activeDegrees);
        triggerNote(currDegree, currOctave, NOTE_OFF);
        break;
    }
}

/**
 * TOGGLE MODE
 * 
 * still needs to be written to handle 3-stage toggle switch.
**/
void TouchChannel::toggleMode()
{
    if (currMode == MONO || currMode == MONO_LOOP)
    {
        setMode(QUANTIZER);
    }
    else
    {
        setMode(MONO);
    }
}

/**
 * TODO: Is this function getting called inside an interrupt?
*/ 
void TouchChannel::onTouch(uint8_t pad)
{
    if (pad < 8)  // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (currMode) {
            case MONO:
                triggerNote(pad, currOctave, NOTE_ON);
                break;
            case MONO_LOOP:
                // create a new event
                if (sequence.recordEnabled)
                {
                    sequence.overdub = true;
                    sequence.createEvent(sequence.currPosition, pad, HIGH);
                }
                // when record is disabled, this block will freeze the sequence and output the curr touched degree until touch is released
                else {
                    sequence.playbackEnabled = false;
                }
                triggerNote(pad, currOctave, NOTE_ON);
                break;
            case QUANTIZER:
                setActiveDegrees(bitWrite(activeDegrees, pad, !bitRead(activeDegrees, pad)));
                break;
            case QUANTIZER_LOOP:
                // every touch detected, take a snapshot of all active degree values and apply them to the sequence
                setActiveDegrees(bitWrite(activeDegrees, pad, !bitRead(activeDegrees, pad)));
                sequence.createChordEvent(sequence.currPosition, activeDegrees);
                break;
            default:
                break;
        }
    }
    else // handle octave pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        setOctave(pad);
    }
}

void TouchChannel::onRelease(uint8_t pad)
{
    if (pad < 8) // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (currMode) {
            case MONO:
                triggerNote(pad, currOctave, NOTE_OFF);
                break;
            case MONO_LOOP:
                if (sequence.recordEnabled)
                {
                    sequence.createEvent(sequence.currPosition, pad, LOW);
                    sequence.overdub = false;
                }
                else
                {
                    sequence.playbackEnabled = true;
                }
                triggerNote(pad, currOctave, NOTE_OFF);
                break;
            case QUANTIZER:
                break;
            case QUANTIZER_LOOP:
                break;
            default:
                break;
        }
    }
    else
    {
        setGate(LOW);
    }
}

/**
 * @param degree an index value between 0..7 that gets mapped to note arrays and pin arrays
 * @param octave index value between 0..3 that gets mapped to note arrays and pin arrays
 * @param action action
*/
void TouchChannel::triggerNote(int degree, int octave, Action action)
{
    // stack the degree, octave, and degree switch state to get an index between 0..DAC_1VO_ARR_SIZE
    int dacIndex = DEGREE_INDEX_MAP[degree] + DAC_OCTAVE_MAP[octave] + degreeSwitches->switchStates[degree];

    prevDegree = currDegree;
    prevOctave = currOctave;

    switch (action) {
        case NOTE_ON:
            if (currMode == MONO || currMode == MONO_LOOP) {
                setDegreeLed(prevDegree, OFF); // set the 'previous' active note led LOW
                setDegreeLed(degree, ON); // new active note HIGH
            }
            setGate(HIGH);
            currDegree = degree;
            currOctave = octave;
            output.updateDAC(dacIndex, 0);
            break;
        case NOTE_OFF:
            setGate(LOW);
            break;
        case SUSTAIN:
            if (currMode == MONO || currMode == MONO_LOOP)
            {
                setDegreeLed(degree, ON);      // new active note HIGH
            }
            output.updateDAC(dacIndex, 0);
            break;
        case PREV_NOTE:
            /* code */
            break;
        case BEND_PITCH:
            /* code */
            break;
    }
}

void TouchChannel::freeze(bool state)
{
    freezeChannel = state;
    if (freezeChannel == true) {
        // log the last sequence led to be illuminated
    } else {
        // turn off the last led in sequence before freeze
    }
}

void TouchChannel::updateDegrees()
{
    switch (currMode)
    {
    case MONO:
        triggerNote(currDegree, currOctave, SUSTAIN);
        break;
    case MONO_LOOP:
        break;
    case QUANTIZER:
        break;
    case QUANTIZER_LOOP:
        break;
    }
}

void TouchChannel::setLED(int io_pin, LedState state)
{
    switch (state) {
        case OFF:
            _leds->setOnTime(io_pin, 0);
            _leds->digitalWrite(io_pin, 1);
            break;
        case ON:
            _leds->setOnTime(io_pin, 0);
            _leds->digitalWrite(io_pin, 0);
            break;
        case BLINK_ON:
            _leds->blinkLED(io_pin, 1, 1, 127, 0);
            break;
        case BLINK_OFF:
            _leds->setOnTime(io_pin, 0);
            break;
        case DIM_LOW:
            _leds->setPWM(io_pin, 40);
            break;
        case DIM_MED:
            _leds->setPWM(io_pin, 127);
            break;
        case DIM_HIGH:
            _leds->setPWM(io_pin, 255);
            break;
        default:
            break;
    }
}

void TouchChannel::setDegreeLed(int degree, LedState state) {
    setLED(DEGREE_LED_PINS[degree], state);
}

void TouchChannel::setOctaveLed(int octave, LedState state) {
    setLED(OCTAVE_LED_PINS[octave], state);
}

void TouchChannel::updateOctaveLeds(int octave)
{
    for (int i = 0; i < OCTAVE_COUNT; i++)
    {
        if (i == octave)
        {
            setOctaveLed(i, ON);
        }
        else
        {
            setOctaveLed(i, OFF);
        }
    }
}

/**
 * take a number between 0 - 3 and apply to currOctave
**/
void TouchChannel::setOctave(int octave)
{
    prevOctave = currOctave;
    currOctave = octave;

    switch (currMode)
    {
    case MONO:
        updateOctaveLeds(currOctave);
        triggerNote(currDegree, currOctave, NOTE_ON);
        break;
    case MONO_LOOP:
        updateOctaveLeds(currOctave);
        triggerNote(currDegree, currOctave, SUSTAIN);
        break;
    case QUANTIZER:
        setActiveOctaves(octave);
        setActiveDegrees(activeDegrees); // update active degrees thresholds
        break;
    case QUANTIZER_LOOP:
        // setActiveOctaves(octave);
        // setActiveDegrees(activeDegrees); // update active degrees thresholds
        break;
    }

    prevOctave = currOctave;
}

/**
 * sets the +5v gate output via GPIO
*/
void TouchChannel::setGate(bool state)
{
    gateState = state;
    gateOut.write(gateState);
    globalGateOut->write(gateState);
}

#define RATCHET_DIV_1 PPQN / 1
#define RATCHET_DIV_2 PPQN / 2
#define RATCHET_DIV_3 PPQN / 3
#define RATCHET_DIV_4 PPQN / 4
#define RATCHET_DIV_6 PPQN / 6
#define RATCHET_DIV_8 PPQN / 8
#define RATCHET_DIV_12 PPQN / 12
#define RATCHET_DIV_16 PPQN / 16

/**
 * @brief Take a 16-bit value and map it to one of the preset Ratchet Rates
*/
uint8_t TouchChannel::calculateRatchet(uint16_t bend)
{
    if (bend > bender->getIdleValue()) // BEND UP == Strait
    {
        if (bend < bender->ratchetThresholds[3])
        {
            return RATCHET_DIV_1;
        }
        else if (bend < bender->ratchetThresholds[2])
        {
            return RATCHET_DIV_2;
        }
        else if (bend < bender->ratchetThresholds[1])
        {
            return RATCHET_DIV_4;
        }
        else if (bend < bender->ratchetThresholds[0])
        {
            return RATCHET_DIV_8;
        }
    }
    else if (bend < bender->getIdleValue())  // BEND DOWN == Triplets
    {
        if (bend > bender->ratchetThresholds[7])
        {
            return RATCHET_DIV_3;
        }
        else if (bend > bender->ratchetThresholds[6])
        {
            return RATCHET_DIV_6;
        }
        else if (bend > bender->ratchetThresholds[5])
        {
            return RATCHET_DIV_12;
        }
        else if (bend > bender->ratchetThresholds[4])
        {
            return RATCHET_DIV_16;
        }
    }
}

/**
 * @brief based on the current position of the sequencer clock, toggle gate on / off
*/
void TouchChannel::handleRatchet(int position, uint8_t rate)
{
    if (position % rate == 0) {
        setGate(HIGH);
        setLED(CHANNEL_RATCHET_LED, ON);
    } else {
        setGate(LOW);
        setLED(CHANNEL_RATCHET_LED, OFF);
    }
}

/**
 * ============================================ 
 * ------------------ BENDER ------------------
*/

/**
 * Set Bender Mode
 * @brief either set bender mode to the supplied target, or just incremement to the next mode
*/
int TouchChannel::setBenderMode(BenderMode targetMode /*INCREMENT_BENDER_MODE*/)
{
    if (targetMode != INCREMENT_BENDER_MODE)
    {
        benderMode = targetMode;
    }
    else if (benderMode < 3)
    {
        benderMode += 1;
    }
    else
    {
        benderMode = 0;
    }
    switch (benderMode)
    {
    case BEND_OFF:
        setLED(CHANNEL_RATCHET_LED, OFF);
        setLED(CHANNEL_PB_LED, OFF);
        break;
    case PITCH_BEND:
        setLED(CHANNEL_RATCHET_LED, OFF);
        setLED(CHANNEL_PB_LED, ON);
        break;
    case RATCHET:
        setLED(CHANNEL_RATCHET_LED, ON);
        setLED(CHANNEL_PB_LED, OFF);
        break;
    case RATCHET_PITCH_BEND:
        setLED(CHANNEL_RATCHET_LED, ON);
        setLED(CHANNEL_PB_LED, ON);
        break;
    case INCREMENT_BENDER_MODE:
        break;
    case BEND_MENU:
        display->setSequenceLEDs(channelIndex, sequence.length, 2, true);
        break;
    }
    return benderMode;
}

/**
 * apply the pitch bend by mapping the ADC value to a value between PB Range value and the current note being outputted
*/
void TouchChannel::benderActiveCallback(uint16_t value)
{
    switch (this->benderMode)
    {
    case BEND_OFF:
        // do nothing
        break;
    case PITCH_BEND:
        uint16_t bend;
        // Pitch Bend UP
        if (bender->currState == Bender::BEND_UP)
        {
            bend = output.calculatePitchBend(value, bender->getIdleValue(), bender->getMaxBend());
            output.setPitchBend(bend); // non-inverted
        }
        // Pitch Bend DOWN
        else if (bender->currState == Bender::BEND_DOWN)
        {
            bend = output.calculatePitchBend(value, bender->getIdleValue(), bender->getMinBend()); // NOTE: inverted mapping
            output.setPitchBend(bend * -1);                                           // inverted
        }
        break;
    case RATCHET:
        currRatchetRate = calculateRatchet(value);
        handleRatchet(sequence.currStepPosition, currRatchetRate);
        break;
    case RATCHET_PITCH_BEND:
        break;
    case BEND_MENU:
        break;
    }
}

void TouchChannel::benderIdleCallback()
{
    switch (this->benderMode)
    {
    case BEND_OFF:
        break;
    case PITCH_BEND:
        output.setPitchBend(0);
        break;
    case RATCHET:
        setLED(CHANNEL_RATCHET_LED, ON);
        break;
    case RATCHET_PITCH_BEND:
        break;
    case BEND_MENU:
        // set var to no longer active?
        break;
    }
}

void TouchChannel::benderTriStateCallback(Bender::BendState state)
{
    switch (this->benderMode)
    {
    case BEND_OFF:
        break;
    case PITCH_BEND:
        break;
    case RATCHET:
        break;
    case RATCHET_PITCH_BEND:
        break;
    case BEND_MENU:
        if (state == Bender::BendState::BEND_UP)
        {
            sequence.setLength(sequence.length + 1);
            display->setSequenceLEDs(this->channelIndex, sequence.length, 2, true);
        }
        else if (state == Bender::BendState::BEND_DOWN)
        {
            display->setSequenceLEDs(this->channelIndex, sequence.length, 2, false);
            sequence.setLength(sequence.length - 1);
            display->setSequenceLEDs(this->channelIndex, sequence.length, 2, true);
        }
        break;
    }
}

/**
 * ============================================ 
 * ------------ QUANTIZATION ------------------
 * 
 * determin which degreese are active / inactive
 * apply the number of active degrees to numActiveDegrees variable
 * read the voltage on analog input pin
 * divide the maximum possible value from analog input pin by the number numActiveDegrees
 * map this value to an array[8]
 * map voltage input values (num between 0 and 1023)
*/

#define CV_MAX 65535

// Init quantizer defaults
void TouchChannel::initQuantizer()
{
    activeDegrees = 0xFF;
    currActiveOctaves = 0xF;
    numActiveDegrees = DEGREE_COUNT;
    numActiveOctaves = OCTAVE_COUNT;
}

void TouchChannel::setActiveDegreeLimit(int value)
{
    activeDegreeLimit = value;
}

void TouchChannel::handleCVInput()
{
    // NOTE: CV voltage input is inverted, so everything needs to be flipped to make more sense
    uint16_t currCVInputValue = CV_MAX - adc.read_u16();

    // We only want trigger events in quantizer mode, so if the gate gets set HIGH, make sure to set it back to low the very next tick
    if (gateState == HIGH)
        setGate(LOW);

    if (currCVInputValue >= prevCVInputValue + CV_QUANTIZER_DEBOUNCE || currCVInputValue <= prevCVInputValue - CV_QUANTIZER_DEBOUNCE)
    {
        int refinedValue = 0; // we want a number between 0 and CV_OCTAVE for mapping to degrees. The octave is added afterwords via CV_OCTAVES
        int octave = 0;

        // determin which octave the CV value will get mapped to
        for (int i = 0; i < numActiveOctaves; i++)
        {
            if (currCVInputValue < activeOctaveValues[i].threshold)
            {
                octave = activeOctaveValues[i].octave;
                refinedValue = i == 0 ? currCVInputValue : currCVInputValue - activeOctaveValues[i - 1].threshold; // remap adc value to a number between 0 and octaveThreshold
                break;
            }
        }

        // latch incoming ADC value to DAC value
        for (int i = 0; i < numActiveDegrees; i++)
        {
            // if the calculated value is less than threshold
            if (refinedValue < activeDegreeValues[i].threshold)
            {
                // prevent duplicate triggering of that same degree / octave
                if (currDegree != activeDegreeValues[i].noteIndex || currOctave != octave) // NOTE: currOctave used to be prevOctave 🤷‍♂️
                {
                    triggerNote(currDegree, prevOctave, NOTE_OFF);  // set previous triggered degree 

                    // re-DIM previously degree LED
                    if (bitRead(activeDegrees, prevDegree))
                    {
                        setDegreeLed(currDegree, BLINK_OFF);
                        setDegreeLed(currDegree, DIM_LOW);
                    }

                    // trigger the new degree, and set its LED to blink
                    triggerNote(activeDegreeValues[i].noteIndex, octave, NOTE_ON);
                    setDegreeLed(activeDegreeValues[i].noteIndex, LedState::BLINK_ON);

                    // re-DIM previous Octave LED
                    if (bitRead(currActiveOctaves, prevOctave))
                    {
                        setOctaveLed(prevOctave, LedState::BLINK_OFF);
                        setOctaveLed(prevOctave, LedState::DIM_LOW);
                    }
                    // BLINK active quantized octave
                    setOctaveLed(octave, LedState::BLINK_ON);
                }
                break; // break from loop as soon as we can
            }
        }
    }
    prevCVInputValue = currCVInputValue;
}

/**
 * @brief when a channels degree is touched, toggle the active/inactive status of the 
 * touched degree by flipping the bit of the given index that was touched
 * @param degrees bit representation of active/inactive degrees
*/
void TouchChannel::setActiveDegrees(uint8_t degrees)
{
    // one degree must always be active
    if (degrees == 0) return;

    activeDegrees = degrees;

    // determine the number of active degrees
    numActiveDegrees = 0;
    for (int i = 0; i < DEGREE_COUNT; i++)
    {
        if (bitRead(activeDegrees, i))
        {
            activeDegreeValues[numActiveDegrees].noteIndex = i;
            numActiveDegrees += 1;
            setDegreeLed(i, DIM_LOW);
            setDegreeLed(i, ON);
        }
        else
        {
            setDegreeLed(i, OFF);
        }
    }

    // determine the number of active octaves (required for calculating thresholds)
    numActiveOctaves = 0;
    for (int i = 0; i < OCTAVE_COUNT; i++)
    {
        if (bitRead(currActiveOctaves, i))
        {
            activeOctaveValues[numActiveOctaves].octave = i;
            numActiveOctaves += 1;
            setOctaveLed(i, DIM_LOW);
            setOctaveLed(i, ON);
        }
        else
        {
            setOctaveLed(i, OFF);
        }
    }
    prevActiveOctaves = currActiveOctaves;

    int octaveThreshold = CV_MAX / numActiveOctaves;        // divide max ADC value by num octaves
    int min_threshold = octaveThreshold / numActiveDegrees; // then numActiveDegrees

    // for each active octave, generate an ADC threshold for mapping ADC values to DAC octave values
    for (int i = 0; i < numActiveOctaves; i++)
    {
        activeOctaveValues[i].threshold = octaveThreshold * (i + 1); // can't multiply by zero
    }

    // for each active degree, generate a ADC 'threshold' for latching CV input values
    for (int i = 0; i < numActiveDegrees; i++)
    {
        activeDegreeValues[i].threshold = min_threshold * (i + 1); // can't multiply by zero
    }
}

/**
 * @brief take the newly touched octave, and either add it or remove it from the currActiveOctaves list
*/
void TouchChannel::setActiveOctaves(int octave)
{
    if (bitFlip(currActiveOctaves, octave) != 0) // one octave must always remain active.
    {
        currActiveOctaves = bitFlip(currActiveOctaves, octave);
    }
}

/**
 * ============================================ 
 * -------------- SEQUENCING ------------------
*/

/**
 * Sequence Handler which gets called during polling / every clocking event
*/
void TouchChannel::handleSequence(int position)
{
    // always disp;ay sequence progresion regardless if there are events or not
    if (sequence.currStep != sequence.prevStep) // why?
    {
        display->stepSequenceLED(this->channelIndex, sequence.currStep, sequence.prevStep, sequence.length);
    }

    // break out if there are no sequence events
    if (sequence.containsEvents == false) {
        return;
    }

    if (sequence.playbackEnabled == false) {
        return;
    }

    switch (currMode)
    {
    case MONO_LOOP:
        if (sequence.getEventStatus(position)) // if event is active
        {
            // Handle Sequence Overdubing
            if (sequence.overdub && position != sequence.newEventPos) // when a node is being created (touched degree has not yet been released), this flag gets set to true so that the sequence handler clears existing nodes
            {
                // if new event overlaps succeeding events, clear those events
                sequence.clearEvent(position);
            }
            // Handle Sequence Events
            else
            {
                if (sequence.getEventGate(position) == HIGH)
                {
                    sequence.prevEventPos = position;                                 // store position into variable
                    triggerNote(sequence.getEventDegree(position), currOctave, NOTE_ON); // trigger note ON
                }
                else
                {
                    // CLEAN UP: if this 'active' LOW node does not match the last active HIGH node, delete it - it is a remnant of a previously deleted node
                    if (sequence.getEventDegree(sequence.prevEventPos) != sequence.getEventDegree(position))
                    {
                        sequence.clearEvent(position);
                    }
                    else // set event.gate LOW
                    {
                        sequence.prevEventPos = position;                         // store position into variable
                        triggerNote(sequence.getEventDegree(position), currOctave, NOTE_OFF); // trigger note OFF
                    }
                }
            }
        }
        break;
    case QUANTIZER_LOOP:
        if (sequence.getEventStatus(position))
        {
            if (sequence.overdub)
            {
                sequence.clearEvent(position);
            }
            else
            {
                setActiveDegrees(sequence.getActiveDegrees(position));
            }
        }
        break;
    }

    triggerNote(currDegree, currOctave, BEND_PITCH); // always handle pitch bend value
}

/**
 * @brief reset the sequence
 * @todo you should probably get the currently queued event, see if it has been triggered yet, and disable it if it has been triggered
 * @todo you can't reset 
*/
void TouchChannel::resetSequence()
{
    sequence.reset();
    if (sequence.containsEvents)
    {
        display->stepSequenceLED(channelIndex, sequence.currStep, sequence.prevStep, sequence.length);
        handleSequence(sequence.currPosition);
    }
}

void TouchChannel::enableSequenceRecording()
{
    sequence.recordEnabled = true;
    display->setSequenceLEDs(channelIndex, sequence.length, 2, true);
    if (currMode == MONO)
    {
        setMode(MONO_LOOP);
    }
    else if (currMode == QUANTIZER)
    {
        setMode(QUANTIZER_LOOP);
    }
}

/**
 * NOTE: A nice feature here would be to only have the LEDs be red when REC is held down, and flash the green LEDs
 * when a channel contains loop events, but REC is NOT held down. You would only be able to add new events to
 * the loop when REC is held down (ie. when channel leds are RED)
 * 
 * NOTE: ADDITIONALLY, this would be a good place to count the amount of steps which have passed while the REC button has
 * been held down, and if this value is greater than the current loop length, update the loop length to accomodate.
 * the new loop length would just increase the multiplier by one
*/
void TouchChannel::disableSequenceRecording()
{
    sequence.recordEnabled = false;
    
    // if a touch event was recorded, remain in loop mode
    if (sequence.containsEvents)
    {
        return;
    }
    else // if no touch event recorded, revert to previous mode
    { 
        display->setSequenceLEDs(channelIndex, sequence.length, 2, false);
        if (currMode == MONO_LOOP)
        {
            setMode(MONO);
        }
        else if (currMode == QUANTIZER_LOOP)
        {
            setMode(QUANTIZER);
        }
    }
}

void TouchChannel::initializeCalibration() {
    output.resetVoltageMap(); // You should reset prior to tuning
    output.resetDAC();
    display->clear();
    display->drawSpiral(channelIndex, true, 50);
    display->clear();
}

/**
 * @brief dedicated high priority task for each touch channel which listens for a notification sent by the touch interrupt
 *
 *
 * @todo Set the MPR121 callback to send a notification
 * @todo setup task to listen for a notification
 * @todo when notification recieive, call handleTouch(), which will read the pads via I2C and then trigger
 * the onTouch and onRelease callbacks, respectively
 * @todo add task to scheduler
 * @param touch_chan_ptr TouchChannel pointer
 */
void TouchChannel::taskHandleTouch(void *touch_chan_ptr) {
    TouchChannel *_this = (TouchChannel*)touch_chan_ptr;
    logger_log_task_watermark();
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        _this->touchPads->handleTouch(); // this will trigger either onTouch() or onRelease()
    }
}

/**
 * @brief send a notification to the handle touch task
 */
void TouchChannel::handleTouchInterrupt() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(this->handleTouchTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}