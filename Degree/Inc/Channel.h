#pragma once

#include "main.h"
#include "MPR121.h"
#include "SX1509.h"

namespace DEGREE {

    #define CHANNEL_REC_LED 11
    #define CHANNEL_RATCHET_LED 10
    #define CHANNEL_PB_LED 9
    #define CHANNEL_QUANT_LED 8
    static const int OCTAVE_LED_PINS[4] = { 3, 2, 1, 0 };               // led driver pin map for octave LEDs
    static const int DEGREE_LED_PINS[8] = { 15, 14, 13, 12, 7, 6, 5, 4 }; // led driver pin map for channel LEDs
    static const int CHAN_TOUCH_PADS[12] = { 7, 6, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0 }; // for mapping touch pads to index values

    class Channel {
    public:
        enum Action {
            NOTE_ON,
            NOTE_OFF,
            SUSTAIN,
            PREV_NOTE,
            BEND_PITCH
        };

        enum ChannelMode {
            MONOPHONIC,
            MONO_LOOP,
            QUANTIZER,
            QUANTIZER_LOOP
        };

        enum LedState {
            LOW,
            HIGH
        };

        Channel(MPR121 *touchPads, SX1509 *leds)
        {
            _touchPads = touchPads;
            _leds = leds;
            _mode = MONOPHONIC;
            _currDegree = 0;
            _currOctave = 0;
        };

        MPR121 *_touchPads;
        SX1509 *_leds;
        ChannelMode _mode;

        uint8_t _currDegree;
        uint8_t _currOctave;
        uint8_t _prevDegree;
        uint8_t _prevOctave;

        void init();
        void poll();

        void onTouch(uint8_t pad);
        void onRelease(uint8_t pad);
        void triggerNote(int degree, int octave, Action action);

        void setLED(int index, LedState state);
    };


} // end namespace

