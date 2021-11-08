#include "TouchChannel.h"

using namespace DEGREE;

extern uint16_t FILTERED_ADC_VALUES[ADC_DMA_BUFF_SIZE];

void TouchChannel::init()
{
    output.init(); // must init this first (for the dac)
    
    bender->init();
    bender->attachActiveCallback(callback(this, &TouchChannel::benderActiveCallback));

    // initialize channel touch pads
    _touchPads->init();
    _touchPads->attachCallbackTouched(callback(this, &TouchChannel::onTouch));
    _touchPads->attachCallbackReleased(callback(this, &TouchChannel::onRelease));
    _touchPads->enable();

    // initialize LED Driver
    _leds->init();
    _leds->setBlinkFrequency(SX1509::ULTRA_FAST);

    for (int i = 0; i < 16; i++) {
        _leds->ledConfig(i);
        setLED(i, LOW);
    }

    triggerNote(currDegree, currOctave, NOTE_ON);
    setLED(OCTAVE_LED_PINS[currOctave], HIGH);
}

void TouchChannel::poll() {
    _touchPads->poll();
    bender->poll();
}

void TouchChannel::onTouch(uint8_t pad)
{
    if (pad < 8)  // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (mode) {
            case MONO:
                triggerNote(pad, currOctave, NOTE_ON);
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
    int dacIndex = DEGREE_INDEX_MAP[degree] + DAC_OCTAVE_MAP[octave] + _degrees->switchStates[degree];

    prevDegree = currDegree;
    prevOctave = currOctave;

    switch (action) {
        case NOTE_ON:
            if (mode == MONO || mode == MONO_LOOP) {
                setLED(DEGREE_LED_PINS[prevDegree], LOW); // set the 'previous' active note led LOW
                setLED(DEGREE_LED_PINS[degree], HIGH); // new active note HIGH
            }
            currDegree = degree;
            currOctave = octave;
            output.updateDAC(dacIndex, 0);
            break;
        case NOTE_OFF:
            /* code */
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
    switch (mode)
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

void TouchChannel::setLED(int index, LedState state)
{
    switch (state) {
        case LOW:
            _leds->setOnTime(index, 0);
            _leds->digitalWrite(index, 1);
            break;
        case HIGH:
            _leds->setOnTime(index, 0);
            _leds->digitalWrite(index, 0);
        default:
            break;
    }
}

/**
 * take a number between 0 - 3 and apply to currOctave
**/
void TouchChannel::setOctave(int octave)
{
    prevOctave = currOctave;
    currOctave = octave;

    switch (mode)
    {
    case MONO:
        setLED(OCTAVE_LED_PINS[prevOctave], LOW);
        setLED(OCTAVE_LED_PINS[octave], HIGH);
        triggerNote(currDegree, currOctave, NOTE_ON);
        break;
    case MONO_LOOP:
        setLED(OCTAVE_LED_PINS[prevOctave], LOW);
        setLED(OCTAVE_LED_PINS[octave], HIGH);
        triggerNote(currDegree, currOctave, SUSTAIN);
        break;
    case QUANTIZER:
        // setActiveOctaves(octave);
        // setActiveDegrees(activeDegrees); // update active degrees thresholds
        break;
    case QUANTIZER_LOOP:
        // setActiveOctaves(octave);
        // setActiveDegrees(activeDegrees); // update active degrees thresholds
        break;
    }

    prevOctave = currOctave;
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