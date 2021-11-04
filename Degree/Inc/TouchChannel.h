#pragma once

#include "main.h"
#include "MPR121.h"
#include "SX1509.h"
#include "DAC8554.h"
#include "Degrees.h"
#include "VoltPerOctave.h"

namespace DEGREE {

    #define CHANNEL_REC_LED 11
    #define CHANNEL_RATCHET_LED 10
    #define CHANNEL_PB_LED 9
    #define CHANNEL_QUANT_LED 8
    static const int OCTAVE_LED_PINS[4] = { 3, 2, 1, 0 };               // led driver pin map for octave LEDs
    static const int DEGREE_LED_PINS[8] = { 15, 14, 13, 12, 7, 6, 5, 4 }; // led driver pin map for channel LEDs
    static const int CHAN_TOUCH_PADS[12] = { 7, 6, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0 }; // for mapping touch pads to index values

    static const int DAC_OCTAVE_MAP[4] = { 0, 12, 24, 36 };               // for mapping a value between 0..3 to octaves
    static const int DEGREE_INDEX_MAP[8] = { 0, 2, 4, 6, 8, 10, 12, 14 }; // for mapping an index between 0..7 to a scale degree

    class TouchChannel {
    public:
        enum Action {
            NOTE_ON,
            NOTE_OFF,
            SUSTAIN,
            PREV_NOTE,
            BEND_PITCH
        };

        enum TouchChannelMode {
            MONOPHONIC,
            MONO_LOOP,
            QUANTIZER,
            QUANTIZER_LOOP
        };

        enum LedState {
            LOW,
            HIGH
        };

        TouchChannel(MPR121 *touchPads, SX1509 *leds, Degrees *degrees, DAC8554 *dac, DAC8554::Channel dac_chan, int pbIndex)
        {
            _touchPads = touchPads;
            _leds = leds;
            _degrees = degrees;
            _mode = MONOPHONIC;
            _currDegree = 0;
            _currOctave = 0;
            _output.dac = dac;
            _output.dacChannel = dac_chan;
            pb_adc_index = pbIndex;
        };

        MPR121 *_touchPads;
        SX1509 *_leds;
        Degrees *_degrees;
        VoltPerOctave _output;

        TouchChannelMode _mode;
        
        int pb_adc_index;

        uint8_t _currDegree;
        uint8_t _currOctave;
        uint8_t _prevDegree;
        uint8_t _prevOctave;

        void init();
        void poll();

        void onTouch(uint8_t pad);
        void onRelease(uint8_t pad);
        void triggerNote(int degree, int octave, Action action);
        void updateDegrees();

        void setLED(int index, LedState state);
    };


} // end namespace

