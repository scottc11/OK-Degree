#include "TouchChannel.h"

using namespace DEGREE;

void TouchChannel::init()
{
    uiMode = UI_PLAYBACK;
    
    output.init(); // must init this first (for the dac)
    
    adc.setFilter(0.1);

    // initialize LED Driver
    _leds->init();
    _leds->setBlinkFrequency(SX1509::ClockSpeed::FAST);

    for (int i = 0; i < 16; i++)
    {
        _leds->ledConfig(i);
        setLED(i, OFF, false);
    }

    bender->adc.attachSamplingProgressCallback(callback(this, &TouchChannel::displayProgressCallback));
    bender->init();
    bender->adc.detachSamplingProgressCallback();
    display->clear(channelIndex);
    bender->attachActiveCallback(callback(this, &TouchChannel::benderActiveCallback));
    bender->attachIdleCallback(callback(this, &TouchChannel::benderIdleCallback));
    bender->attachTriStateCallback(callback(this, &TouchChannel::benderTriStateCallback));

    sequence.init(); // really important sequencer initializes after the bender gets initialized

    // initialize channel touch pads
    touchPads->init();
    touchPads->attachInterruptCallback(callback(this, &TouchChannel::handleTouchInterrupt));
    touchPads->attachCallbackTouched(callback(this, &TouchChannel::onTouch));
    touchPads->attachCallbackReleased(callback(this, &TouchChannel::onRelease));
    touchPads->enable();

    // you should actually be accessing a global settings buffer 
    display->drawSpiral(channelIndex, true, 25);
    setPlaybackMode(playbackMode); // value of playbackMode gets loaded and assigned from flash
    setBenderMode((BenderMode)currBenderMode); // value of playbackMode gets loaded and assigned from flash
    logPeripherals();
}

/** ------------------------------------------------------------------------
 *         POLL    POLL    POLL    POLL    POLL    POLL    POLL    POLL    
 *  ------------------------------------------------------------------------
*/
void TouchChannel::poll()
{
    switch (uiMode)
    {
    case UI_PLAYBACK:
        break;
    case UI_PITCH_BEND_RANGE:
        break;
    case UI_QUANTIZE_AMOUNT:
        break;
    case UI_SEQUENCE_LENGTH:
        if (tickerFlag)
        {
            bender->poll();
            clearTickerFlag();
        }
    }
}

/**
 * @brief gets called once every PPQN automatically by a task
 *
 */
void TouchChannel::handleClock() {
    if (!freezeChannel)
    {
        bender->poll();

        if (playbackMode == QUANTIZER || playbackMode == QUANTIZER_LOOP)
        {
            handleCVInput();
        }

        if (playbackMode == MONO_LOOP || playbackMode == QUANTIZER_LOOP)
        {
            handleSequence(sequence.currPosition);
        }
    }
}

void TouchChannel::setUIMode(UIMode targetMode) {
    this->uiMode = targetMode;
    switch (targetMode)
    {
    case UIMode::UI_PLAYBACK:
        setPlaybackMode(playbackMode);
        break;
    case UIMode::UI_PITCH_BEND_RANGE:
        setAllOctaveLeds(LedState::OFF, false);
        handlePitchBendRangeUI();
        break;
    case UI_SEQUENCE_LENGTH:
        break;
    case UI_QUANTIZE_AMOUNT:
        handleQuantizeAmountUI();
        break;
    }
}

/**
 * @brief determine current pitch bend range index and update LEDs accordingly
*/
void TouchChannel::handlePitchBendRangeUI()
{
    setAllDegreeLeds(LedState::OFF, false);
    for (int i = 0; i < output.getPitchBendRange() + 1; i++)
    {
        int inversion = 7 - i; // invert
        setDegreeLed(i, LedState::ON, false);
    }
}

/**
 * @brief what if you flashed the degree leds at a rate relative to their corrosponding value
 * ie. if you want to quantize to an 8th note grid, then you would touch the degree LED that is flashing
 * at a rate of 8th notes (relative to clock)
 */
void TouchChannel::handleQuantizeAmountUI()
{
    setAllOctaveLeds(LedState::OFF, false);
    setAllDegreeLeds(LedState::OFF, false);
}

/**
 * @brief set the channels mode
*/ 
void TouchChannel::setPlaybackMode(PlaybackMode targetMode)
{
    playbackMode = targetMode;

    // start from a clean slate by setting all the LEDs LOW
    for (int i = 0; i < DEGREE_COUNT; i++) {
        setDegreeLed(i, DIM_MED, false);
        setDegreeLed(i, OFF, false);
    }
    setLED(CHANNEL_REC_LED, OFF, false);
    setLED(CHANNEL_QUANT_LED, OFF, false);
    sequence.playbackEnabled = false;

    switch (playbackMode)
    {
    case MONO:
        triggerNote(currDegree, currOctave, SUSTAIN);
        updateOctaveLeds(currOctave, false);
        break;
    case MONO_LOOP:
        sequence.playbackEnabled = true;
        setLED(CHANNEL_REC_LED, ON, false);
        triggerNote(currDegree, currOctave, SUSTAIN);
        break;
    case QUANTIZER:
        setLED(CHANNEL_QUANT_LED, ON, false);
        setActiveDegrees(activeDegrees);
        triggerNote(currDegree, currOctave, NOTE_OFF);
        break;
    case QUANTIZER_LOOP:
        sequence.playbackEnabled = true;
        setLED(CHANNEL_REC_LED, ON, false);
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
    if (playbackMode == MONO || playbackMode == MONO_LOOP)
    {
        setPlaybackMode(QUANTIZER);
    }
    else
    {
        setPlaybackMode(MONO);
    }
}

/**
 * @brief Called when a pad is touched
 *
 * @param pad pad that was touched
 */
void TouchChannel::onTouch(uint8_t pad)
{
    switch (uiMode)
    {
    case UIMode::UI_PLAYBACK:
        handleTouchPlaybackEvent(pad);
        break;
    case UIMode::UI_SEQUENCE_LENGTH:
        break;
    case UIMode::UI_QUANTIZE_AMOUNT:
        break;
    case UIMode::UI_PITCH_BEND_RANGE:
        // take incoming pad and update the pitch bend range accordingly.
        output.setPitchBendRange(CHAN_TOUCH_PADS[pad]); // this applies the inverse of the pad (ie. pad = 7, gets mapped to 0)
        handlePitchBendRangeUI();
        break;
    }
}

void TouchChannel::onRelease(uint8_t pad)
{
    switch (uiMode)
    {
    case UIMode::UI_PLAYBACK:
        handleReleasePlaybackEvent(pad);
        break;
    case UIMode::UI_SEQUENCE_LENGTH:
        break;
    case UIMode::UI_QUANTIZE_AMOUNT:
        break;
    case UIMode::UI_PITCH_BEND_RANGE:
        break;
    }
}

void TouchChannel::handleTouchUIEvent(uint8_t pad) {

}

void TouchChannel::handleTouchPlaybackEvent(uint8_t pad)
{
    if (pad < 8)  // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (playbackMode) {
            case MONO:
                triggerNote(pad, currOctave, NOTE_ON);
                break;
            case MONO_LOOP:
                // create a new event
                if (sequence.recordEnabled)
                {
                    sequence.overdub = true;
                    sequence.createTouchEvent(sequence.currPosition, pad, HIGH);
                }
                // when record is disabled, this block will freeze the sequence and output the curr touched degree until touch is released
                else {
                    sequence.playbackEnabled = false;
                }
                triggerNote(pad, currOctave, NOTE_ON);
                break;
            case QUANTIZER:
                setActiveDegrees(bitwise_write_bit(activeDegrees, pad, !bitwise_read_bit(activeDegrees, pad)));
                break;
            case QUANTIZER_LOOP:
                // every touch detected, take a snapshot of all active degree values and apply them to the sequence
                setActiveDegrees(bitwise_write_bit(activeDegrees, pad, !bitwise_read_bit(activeDegrees, pad)));
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

void TouchChannel::handleReleasePlaybackEvent(uint8_t pad)
{
    if (pad < 8) // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (playbackMode)
        {
        case MONO:
            triggerNote(pad, currOctave, NOTE_OFF);
            break;
        case MONO_LOOP:
            if (sequence.recordEnabled)
            {
                sequence.createTouchEvent(sequence.currPosition, pad, LOW);
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
            if (playbackMode == MONO || playbackMode == MONO_LOOP) {
                setDegreeLed(prevDegree, OFF, true); // set the 'previous' active note led LOW
                setDegreeLed(degree, ON, true); // new active note HIGH
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
            if (playbackMode == MONO || playbackMode == MONO_LOOP)
            {
                setDegreeLed(degree, ON, true);      // new active note HIGH
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
        freezeStep = sequence.currStep; // log the last sequence led to be illuminated
        // maybe blink the degree LEDs?
        // maybe blink the display LED sequence was frozen at?
    } else {
        // reset last led in sequence before freeze
        if (sequence.playbackEnabled && sequence.currStep != freezeStep)
        {
            setSequenceLED(freezeStep, PWM::PWM_LOW_MID);
        }
    }
}

void TouchChannel::updateDegrees()
{
    switch (playbackMode)
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

void TouchChannel::setLED(int io_pin, LedState state, bool isPlaybackEvent)
{
    if (isPlaybackEvent && uiMode != UI_PLAYBACK)
    {
        if (uiMode == UI_PITCH_BEND_RANGE || uiMode == UI_QUANTIZE_AMOUNT)
        {
            return;
        }
    }
    
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

void TouchChannel::setDegreeLed(int degree, LedState state, bool isPlaybackEvent)
{
    setLED(DEGREE_LED_PINS[degree], state, isPlaybackEvent);
}

void TouchChannel::setOctaveLed(int octave, LedState state, bool isPlaybackEvent)
{
    setLED(OCTAVE_LED_PINS[octave], state, isPlaybackEvent);
}

/**
 * @brief set all degree leds
 */
void TouchChannel::setAllDegreeLeds(LedState state, bool isPlaybackEvent)
{
    for (int i = 0; i < DEGREE_COUNT; i++)
    {
        setDegreeLed(i, state, isPlaybackEvent);
    }
}

/**
 * @brief set all octave leds
 */
void TouchChannel::setAllOctaveLeds(LedState state, bool isPlaybackEvent)
{
    for (int i = 0; i < OCTAVE_COUNT; i++)
    {
        setOctaveLed(i, state, isPlaybackEvent);
    }
}

void TouchChannel::updateOctaveLeds(int octave, bool isPlaybackEvent)
{
    for (int i = 0; i < OCTAVE_COUNT; i++)
    {
        if (i == octave)
        {
            setOctaveLed(i, ON, isPlaybackEvent);
        }
        else
        {
            setOctaveLed(i, OFF, isPlaybackEvent);
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

    switch (playbackMode)
    {
    case MONO:
        updateOctaveLeds(currOctave, true);
        triggerNote(currDegree, currOctave, NOTE_ON);
        break;
    case MONO_LOOP:
        updateOctaveLeds(currOctave, true);
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
void TouchChannel::handleRatchet(int position, uint16_t value)
{
    currRatchetRate = calculateRatchet(value);
    if (position % currRatchetRate == 0)
    {
        setGate(HIGH);
        setLED(CHANNEL_RATCHET_LED, ON, true);
    }
    else
    {
        setGate(LOW);
        setLED(CHANNEL_RATCHET_LED, OFF, true);
    }
}

/**
 * ============================================ 
 * ------------------ BENDER ------------------
*/

void TouchChannel::handleBend(uint16_t value) {
    switch (this->currBenderMode)
    {
    case BEND_OFF:
        // do nothing, bender instance will update its DAC on its own
        break;
    case PITCH_BEND:
        handlePitchBend(value);
        break;
    case RATCHET:
        handleRatchet(sequence.currStepPosition, value);
        break;
    case RATCHET_PITCH_BEND:
        handleRatchet(sequence.currStepPosition, value);
        handlePitchBend(value);
        break;
    case BEND_MENU:
        break;
    }
}

void TouchChannel::handlePitchBend(uint16_t value) {
    if (!touchPads->padIsTouched()) // only apply pitch bend when all pads have been released
    {
        uint16_t pitchbend;
        // Pitch Bend UP
        if (bender->currState == Bender::BENDING_UP)
        {
            pitchbend = output.calculatePitchBend(value, bender->getIdleValue(), bender->getMaxBend());
            output.setPitchBend(pitchbend * -1); // NOTE: inverted mapping
        }
        // Pitch Bend DOWN
        else if (bender->currState == Bender::BENDING_DOWN)
        {
            pitchbend = output.calculatePitchBend(value, bender->getIdleValue(), bender->getMinBend());
            output.setPitchBend(pitchbend);
        }
    }
    
}

/**
 * Set Bender Mode
 * @brief either set bender mode to the supplied target, or just incremement to the next mode
*/
int TouchChannel::setBenderMode(BenderMode targetMode /*INCREMENT_BENDER_MODE*/)
{
    if (targetMode != INCREMENT_BENDER_MODE || targetMode != BEND_MENU) {
        prevBenderMode = currBenderMode;
    }

    if (targetMode != INCREMENT_BENDER_MODE)
    {
        currBenderMode = targetMode;
    }
    else if (currBenderMode < 3)
    {
        currBenderMode += 1;
    }
    else
    {
        currBenderMode = 0;
    }
    switch (currBenderMode)
    {
    case BEND_OFF:
        setLED(CHANNEL_RATCHET_LED, OFF, false);
        setLED(CHANNEL_PB_LED, OFF, false);
        break;
    case PITCH_BEND:
        setLED(CHANNEL_RATCHET_LED, OFF, false);
        setLED(CHANNEL_PB_LED, ON, false);
        break;
    case RATCHET:
        setLED(CHANNEL_RATCHET_LED, ON, false);
        setLED(CHANNEL_PB_LED, OFF, false);
        break;
    case RATCHET_PITCH_BEND:
        setLED(CHANNEL_RATCHET_LED, ON, false);
        setLED(CHANNEL_PB_LED, ON, false);
        break;
    case INCREMENT_BENDER_MODE:
        break;
    case BEND_MENU:
        break;
    }
    return currBenderMode;
}

/**
 * @brief callback that executes when a bender is not in its idle state
 * 
 * @param value the raw ADC value from the bender instance
 */
void TouchChannel::benderActiveCallback(uint16_t value)
{
    bender->updateDAC(value);

    // overdub existing bend events when record enabled
    // override existng bend events when record disabled (but sequencer still ON)
    if (sequence.recordEnabled)
    {
        sequence.createBendEvent(sequence.currPosition, value);
    }
    this->handleBend(value);
}

void TouchChannel::benderIdleCallback()
{
    if (sequence.playbackEnabled)
    {
        if (!sequence.containsBendEvents)
        {
            bender->updateDAC(bender->currBend); // currBend gets set to ist idle value in the underlying handler
        }
    } else {
        bender->updateDAC(bender->currBend);
    }
    

    switch (this->currBenderMode) // do you need this? Or can you call handleBend()?
    {
    case BEND_OFF:
        break;
    case PITCH_BEND:
        output.setPitchBend(0);
        break;
    case RATCHET:
        setLED(CHANNEL_RATCHET_LED, ON, true);
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
    switch (this->currBenderMode)
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
        if (state == Bender::BendState::BENDING_UP)
        {
            dispatch_sequencer_event((CHAN)channelIndex, SEQ::SET_LENGTH, sequence.length + 2);
        }
        else if (state == Bender::BendState::BENDING_DOWN)
        {
            dispatch_sequencer_event((CHAN)channelIndex, SEQ::SET_LENGTH, sequence.length - 2);
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
                    if (bitwise_read_bit(activeDegrees, prevDegree))
                    {
                        setDegreeLed(currDegree, BLINK_OFF, true);
                        setDegreeLed(currDegree, DIM_LOW, true);
                    }

                    // trigger the new degree, and set its LED to blink
                    triggerNote(activeDegreeValues[i].noteIndex, octave, NOTE_ON);
                    setDegreeLed(activeDegreeValues[i].noteIndex, LedState::BLINK_ON, true);

                    // re-DIM previous Octave LED
                    if (bitwise_read_bit(currActiveOctaves, prevOctave))
                    {
                        setOctaveLed(prevOctave, LedState::BLINK_OFF, true);
                        setOctaveLed(prevOctave, LedState::DIM_LOW, true);
                    }
                    // BLINK active quantized octave
                    setOctaveLed(octave, LedState::BLINK_ON, true);
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
        if (bitwise_read_bit(activeDegrees, i))
        {
            activeDegreeValues[numActiveDegrees].noteIndex = i;
            numActiveDegrees += 1;
            setDegreeLed(i, DIM_LOW, false);
            setDegreeLed(i, ON, false);
        }
        else
        {
            setDegreeLed(i, OFF, false);
        }
    }

    // determine the number of active octaves (required for calculating thresholds)
    numActiveOctaves = 0;
    for (int i = 0; i < OCTAVE_COUNT; i++)
    {
        if (bitwise_read_bit(currActiveOctaves, i))
        {
            activeOctaveValues[numActiveOctaves].octave = i;
            numActiveOctaves += 1;
            setOctaveLed(i, DIM_LOW, false);
            setOctaveLed(i, ON, false);
        }
        else
        {
            setOctaveLed(i, OFF, false);
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
    if (bitwise_flip_bit(currActiveOctaves, octave) != 0) // one octave must always remain active.
    {
        currActiveOctaves = bitwise_flip_bit(currActiveOctaves, octave);
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
        stepSequenceLED(sequence.currStep, sequence.prevStep, sequence.length);
    }

    // break out if there are no sequence events
    if (sequence.containsEvents() == false) {
        return;
    }

    if (sequence.playbackEnabled == false) {
        return;
    }

    // Handle Touch Events (degrees)
    if (sequence.containsTouchEvents)
    {
        switch (playbackMode)
        {
        case MONO:
            break;
        case QUANTIZER:
            break;
        case MONO_LOOP:
            if (sequence.getEventStatus(position)) // if event is active
            {
                // Handle Sequence Overdubing
                if (sequence.overdub && position != sequence.newEventPos) // when a node is being created (touched degree has not yet been released), this flag gets set to true so that the sequence handler clears existing nodes
                {
                    // if new event overlaps succeeding events, clear those events
                    sequence.clearTouchAtPosition(position);
                }
                else // Handle Sequence Events
                {
                    if (sequence.getEventGate(position) == HIGH)
                    {
                        sequence.prevEventPos = position;                                    // store position into variable
                        triggerNote(sequence.getEventDegree(position), currOctave, NOTE_ON); // trigger note ON
                    }
                    else // set event.gate LOW
                    {
                        sequence.prevEventPos = position;                                     // store position into variable
                        triggerNote(sequence.getEventDegree(position), currOctave, NOTE_OFF); // trigger note OFF
                    }
                }
            }
            break;
        case QUANTIZER_LOOP:
            if (sequence.getEventStatus(position))
            {
                if (sequence.overdub)
                {
                    sequence.clearTouchAtPosition(position);
                }
                else
                {
                    setActiveDegrees(sequence.getActiveDegrees(position));
                }
            }
            break;
        }
    }

    // Handle Bend Events
    if (sequence.containsBendEvents)
    {
        if (bender->isIdle())
        {
            this->handleBend(sequence.getBend(position));
            this->bender->updateDAC(sequence.getBend(position));
        }
    }
}

/**
 * @brief reset the sequence
 * @todo you should probably get the currently queued event, see if it has been triggered yet, and disable it if it has been triggered
 * @todo you can't reset 
*/
void TouchChannel::resetSequence()
{
    sequence.reset();
    if (sequence.containsEvents())
    {
        stepSequenceLED(sequence.currStep, sequence.prevStep, sequence.length);
        handleSequence(sequence.currPosition);
    }
}

/**
 * @brief set the sequence length and update UI
 *
 * @param length
 */
void TouchChannel::updateSequenceLength(uint8_t steps)
{
    sequence.setLength(steps);
    drawSequenceToDisplay();
}

/**
 * @brief given a sequence step, 
 * 
 * @param step 
 */
void TouchChannel::setSequenceLED(uint8_t step, uint8_t pwm)
{
    uint8_t ledIndex = step / 2; // 32 step seq displayed with 16 LEDs
    display->setChannelLED(channelIndex, ledIndex, pwm); // it is possible ledIndex needs to be subracted by 1 🤔
}

/**
 * @brief illuminates the number of LEDs equal to sequence length divided by 2
 */
void TouchChannel::drawSequenceToDisplay()
{
    for (int i = 0; i < MAX_SEQ_LENGTH; i += 2)
    {
        if (i < sequence.length)
        {
            if (i == sequence.currStep && sequence.playbackEnabled)
            {
                setSequenceLED(i, PWM::PWM_HIGH);
            }
            else
            {
                setSequenceLED(i, PWM::PWM_LOW_MID);
            }
        }
        else
        {
            setSequenceLED(i, PWM::PWM_OFF);
        }
    }
}

void TouchChannel::stepSequenceLED(int currStep, int prevStep, int length)
{
    if (currStep % 2 == 0)
    {
        // set currStep PWM High
        setSequenceLED(currStep, PWM::PWM_HIGH);

        // handle odd sequence lengths.
        //  The last LED in sequence gets set to a different PWM
        if (prevStep == length - 1 && length % 2 == 1)
        {
            setSequenceLED(prevStep, PWM::PWM_LOW);
        }
        // regular sequence lengths
        else
        {
            // set prevStep PWM back to Mid
            setSequenceLED(prevStep, PWM::PWM_LOW_MID);
        }
    }
}

void TouchChannel::enableSequenceRecording()
{
    sequence.recordEnabled = true;
    drawSequenceToDisplay();
    if (playbackMode == MONO)
    {
        setPlaybackMode(MONO_LOOP);
    }
    else if (playbackMode == QUANTIZER)
    {
        setPlaybackMode(QUANTIZER_LOOP);
    }
}

/**
 * NOTE: ADDITIONALLY, this would be a good place to count the amount of steps which have passed while the REC button has
 * been held down, and if this value is greater than the current loop length, update the loop length to accomodate.
 * the new loop length would just increase the multiplier by one
*/
void TouchChannel::disableSequenceRecording()
{
    sequence.recordEnabled = false;
    
    // if a touch event was recorded, remain in loop mode
    if (sequence.containsEvents())
    {
        return;
    }
    else // if no touch event recorded, revert to previous mode
    {
        if (playbackMode == MONO_LOOP)
        {
            setPlaybackMode(MONO);
        }
        else if (playbackMode == QUANTIZER_LOOP)
        {
            setPlaybackMode(QUANTIZER);
        }
        display->clear(channelIndex);
    }
}

void TouchChannel::initializeCalibration() {
    output.resetVoltageMap(); // You should reset prior to tuning
    output.resetDAC();
    display->clear();
    display->drawSpiral(channelIndex, true, 0);
    display->flash(3, 200);
    display->clear();
}

/**
 * @brief ISR from touch IC to send a notification sequencer queue
 */
void TouchChannel::handleTouchInterrupt() {
    dispatch_sequencer_event_ISR((CHAN)channelIndex, SEQ::HANDLE_TOUCH, 0);
}

void TouchChannel::displayProgressCallback(uint16_t progress)
{
    // map the incoming progress to a value between 0..16
    uint8_t displayProgress = map_num_in_range<uint16_t>(progress, 0, ADC_SAMPLE_COUNTER_LIMIT, 0, 15);
    this->display->setChannelLED(this->channelIndex, displayProgress, PWM::PWM_MID_HIGH);
    this->setLED(DEGREE_LED_RAINBOW[displayProgress], LedState::ON, false);
    uint16_t dacProgress = map_num_in_range<uint16_t>(progress, 0, ADC_SAMPLE_COUNTER_LIMIT, 0, BIT_MAX_16);
    bender->dac->write(bender->dacChan, dacProgress);
    if (displayProgress == 15)
    {
        bender->dac->write(bender->dacChan, BENDER_DAC_ZERO);
    }
    
}

void TouchChannel::logPeripherals() {
    logger_log("\n** Channel ");
    logger_log(this->channelIndex);
    logger_log(" **");
    logger_log("\nSX1509 connected: ");
    logger_log(_leds->isConnected());
    logger_log("\nGate Out state: ");
    logger_log(gateOut.read());
    logger_log("\nMPR121 Int Pin: ");
    logger_log(touchPads->readInterruptPin());

    logger_log("\nBender Mode: ");
    logger_log(currBenderMode);
    logger_log("\nBender ADC Value: ");
    logger_log(bender->adc.read_u16());
    bender->adc.log_noise_threshold_to_console("Bender");
    logger_log("\nBender Min Bend: ");
    logger_log(bender->adc.getInputMin());
    logger_log("\nBender Max Bend: ");
    logger_log(bender->adc.getInputMax());

    logger_log("\nPitch Bend Range (index): ");
    logger_log(output.getPitchBendRange());

    logger_log("\nSequence Length: ");
    logger_log(sequence.length);

    logger_log("\nSequence Quantization: ");
    logger_log((int)sequence.quantizeAmount);
    logger_log("\n");
}