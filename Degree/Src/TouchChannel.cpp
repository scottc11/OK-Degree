#include "TouchChannel.h"

using namespace DEGREE;

void TouchChannel::init()
{
    output.init(); // must init this first (for the dac)
    
    sequence.init();

    bender->init();
    bender->attachActiveCallback(callback(this, &TouchChannel::benderActiveCallback));
    bender->attachIdleCallback(callback(this, &TouchChannel::benderIdleCallback));

    // initialize channel touch pads
    _touchPads->init();
    _touchPads->attachCallbackTouched(callback(this, &TouchChannel::onTouch));
    _touchPads->attachCallbackReleased(callback(this, &TouchChannel::onRelease));
    _touchPads->enable();

    // initialize LED Driver
    _leds->init();
    _leds->setBlinkFrequency(SX1509::FAST);

    for (int i = 0; i < 16; i++) {
        _leds->ledConfig(i);
        setLED(i, OFF);
    }

    setMode(MONO);
}

void TouchChannel::poll() {
    _touchPads->poll();
    bender->poll();
    if (currMode == QUANTIZER) {
        handleCVInput();
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

    switch (currMode)
    {
    case MONO:
        triggerNote(currDegree, currOctave, NOTE_ON);
        updateOctaveLeds(currOctave);
        break;
    case MONO_LOOP:
        break;
    case QUANTIZER:
        setLED(CHANNEL_QUANT_LED, ON);
        setActiveDegrees(activeDegrees);
        break;
    case QUANTIZER_LOOP:
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

void TouchChannel::onTouch(uint8_t pad)
{
    if (pad < 8)  // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (currMode) {
            case MONO:
                triggerNote(pad, currOctave, NOTE_ON);
                break;
            case QUANTIZER:
                setActiveDegrees(bitWrite(activeDegrees, pad, !bitRead(activeDegrees, pad)));
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
            bend = output.calculatePitchBend(value, bender->zeroBend, bender->maxBend);
            output.setPitchBend(bend); // non-inverted
        }
        // Pitch Bend DOWN
        else if (bender->currState == Bender::BEND_DOWN)
        {
            bend = output.calculatePitchBend(value, bender->zeroBend, bender->minBend); // NOTE: inverted mapping
            output.setPitchBend(bend * -1);                                           // inverted
        }
        break;
    case RATCHET:
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
        break;
    case RATCHET_PITCH_BEND:
        break;
    case BEND_MENU:
        // set var to no longer active?
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