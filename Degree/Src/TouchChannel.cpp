#include "TouchChannel.h"

using namespace DEGREE;

void TouchChannel::init()
{
    _output.init(); // must init this first (for the dac)

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

    triggerNote(_currDegree, _currOctave, NOTE_ON);
    setLED(OCTAVE_LED_PINS[_currOctave], HIGH);
}

void TouchChannel::poll() {
    _touchPads->poll();
}

void TouchChannel::onTouch(uint8_t pad)
{
    if (pad < 8)  // handle degree pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (_mode) {
            case MONOPHONIC:
                triggerNote(pad, _currOctave, NOTE_ON);
                break;
            default:
                break;
        }
    }
    else // handle octave pads
    {
        pad = CHAN_TOUCH_PADS[pad]; // remap
        switch (_mode) {
            case MONOPHONIC:
                setLED(OCTAVE_LED_PINS[pad], HIGH);
                break;
            default:
                break;
        }
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

    _prevDegree = _currDegree;
    _prevOctave = _currOctave;

    switch (action) {
        case NOTE_ON:
            if (_mode == MONOPHONIC || _mode == MONO_LOOP) {
                setLED(DEGREE_LED_PINS[_prevDegree], LOW); // set the 'previous' active note led LOW
                setLED(DEGREE_LED_PINS[degree], HIGH); // new active note HIGH
            }
            _currDegree = degree;
            _currOctave = octave;
            _output.updateDAC(dacIndex, 0);
            break;
        case NOTE_OFF:
            /* code */
            break;
        case SUSTAIN:
            _output.updateDAC(dacIndex, 0);
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
    switch (_mode)
    {
    case MONOPHONIC:
        triggerNote(_currDegree, _currOctave, SUSTAIN);
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