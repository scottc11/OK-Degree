#pragma once

#include "main.h"
#include "MPR121.h"
#include "SX1509.h"
#include "DAC8554.h"
#include "Degrees.h"
#include "Bender.h"
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
        enum Action
        {
            NOTE_ON,
            NOTE_OFF,
            SUSTAIN,
            PREV_NOTE,
            BEND_PITCH
        };
        
        enum BenderMode
        {
            BEND_OFF = 0,
            PITCH_BEND = 1,
            RATCHET = 2,
            RATCHET_PITCH_BEND = 3,
            INCREMENT_BENDER_MODE = 4,
            BEND_MENU = 5
        };

        enum TouchChannelMode
        {
            MONO,
            MONO_LOOP,
            QUANTIZER,
            QUANTIZER_LOOP
        };

        enum LedState
        {
            LOW,
            HIGH
        };

        TouchChannel(MPR121 *touchPads, SX1509 *leds, Degrees *degrees, DAC8554 *dac, DAC8554::Channel dac_chan, Bender *_bender)
        {
            _touchPads = touchPads;
            _leds = leds;
            _degrees = degrees;
            output.dac = dac;
            output.dacChannel = dac_chan;
            bender = _bender;
            mode = MONO;
            benderMode = PITCH_BEND;
            currDegree = 0;
            currOctave = 0;
        };

        MPR121 *_touchPads;
        SX1509 *_leds;
        Degrees *_degrees;
        Bender *bender;
        VoltPerOctave output;

        TouchChannelMode mode;
        BenderMode benderMode;
        
        int pb_adc_index;

        uint8_t currDegree;
        uint8_t currOctave;
        uint8_t prevDegree;
        uint8_t prevOctave;

        void init();
        void poll();

        void onTouch(uint8_t pad);
        void onRelease(uint8_t pad);
        void triggerNote(int degree, int octave, Action action);
        void updateDegrees();

        void setOctave(int octave);

        void setLED(int index, LedState state);

        // Bender Callbacks
        void benderActiveCallback(uint16_t value);
    };


} // end namespace

