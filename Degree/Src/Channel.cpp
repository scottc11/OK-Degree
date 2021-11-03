#include "Channel.h"

using namespace DEGREE;

void Channel::init()
{
    // initialize channel touch pads
    _touchPads->init();
    _touchPads->attachCallbackTouched(callback(this, &Channel::onTouch));
    _touchPads->attachCallbackReleased(callback(this, &Channel::onRelease));
    _touchPads->enable();

    // initialize LED Driver
    _leds->init();
    _leds->setBlinkFrequency(SX1509::ULTRA_FAST);

    _leds->ledConfig(CHANNEL_PB_LED);
    _leds->ledConfig(CHANNEL_REC_LED);
    _leds->ledConfig(CHANNEL_RATCHET_LED);
    _leds->ledConfig(CHANNEL_QUANT_LED);

    for (int i = 0; i < 8; i++)
    {
        _leds->ledConfig(DEGREE_LED_PINS[i]);
    }

    for (int i = 0; i < 4; i++)
    {
        _leds->ledConfig(OCTAVE_LED_PINS[i]);
    }
    for (int i = 0; i < 16; i++)
    {
        setLED(i, LOW);
    }
}

void Channel::poll() {
    _touchPads->poll();
}

void Channel::onTouch(uint8_t pad)
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

void Channel::onRelease(uint8_t pad)
{

}

void Channel::triggerNote(int degree, int octave, Action action)
{
    _prevDegree = _currDegree;
    _prevOctave = _currOctave;
    switch (action) {
        case NOTE_ON:
            if (_mode == MONOPHONIC || _mode == MONO_LOOP) {
                setLED(DEGREE_LED_PINS[_currDegree], LOW); // set the 'previous' active note led LOW
                setLED(DEGREE_LED_PINS[degree], HIGH); // new active note HIGH
            }
            _currDegree = degree;
            _currOctave = octave;
            break;
        case NOTE_OFF:
            /* code */
            break;
        case SUSTAIN:
            /* code */
            break;
        case PREV_NOTE:
            /* code */
            break;
        case BEND_PITCH:
            /* code */
            break;
    }
}

void Channel::setLED(int index, LedState state)
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