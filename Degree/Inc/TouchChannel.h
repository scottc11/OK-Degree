#pragma once

#include "main.h"
#include "MPR121.h"
#include "SX1509.h"
#include "DAC8554.h"
#include "Degrees.h"
#include "Bender.h"
#include "VoltPerOctave.h"
#include "SuperSeq.h"
#include "SuperClock.h"
#include "Display.h"
#include "okSemaphore.h"
#include "task_sequence_handler.h"

typedef struct QuantOctave
{
    int threshold;
    int octave;
} QuantOctave;

typedef struct QuantDegree
{
    int threshold;
    int noteIndex;
} QuantDegree;

namespace DEGREE {

    #define CHANNEL_REC_LED 11
    #define CHANNEL_RATCHET_LED 10
    #define CHANNEL_PB_LED 9
    #define CHANNEL_QUANT_LED 8
    #define CV_QUANTIZER_DEBOUNCE 1000 // used to avoid rapid re-triggering of a degree when the CV signal is noisy

    static const int OCTAVE_LED_PINS[4] = { 3, 2, 1, 0 };               // led driver pin map for octave LEDs
    static const int DEGREE_LED_PINS[8] = { 15, 14, 13, 12, 7, 6, 5, 4 }; // led driver pin map for channel LEDs
    static const int DEGREE_LED_RAINBOW[16] = { 15, 14, 13, 12, 7, 6, 5, 4, 3, 2, 1, 0, CHANNEL_PB_LED, CHANNEL_RATCHET_LED, CHANNEL_REC_LED, CHANNEL_QUANT_LED };
    static const int CHAN_TOUCH_PADS[12] = { 7, 6, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0 }; // for mapping touch pads to index values
    static const int QUANTIZATION_LED_INDEX_MAP[QUANT_NUM_OPTIONS] = { 1, 2, 3, 4, 5, 6 };

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
        
        enum BenderMode : int
        {
            BEND_OFF = 0,
            PITCH_BEND = 1,
            RATCHET = 2,
            RATCHET_PITCH_BEND = 3,
            INCREMENT_BENDER_MODE = 4
        };

        enum UIMode {
            UI_PLAYBACK,
            UI_PITCH_BEND_RANGE,
            UI_SEQUENCE_LENGTH,
            UI_QUANTIZE_AMOUNT
        };

        enum PlaybackMode
        {
            MONO,
            MONO_LOOP,
            QUANTIZER,
            QUANTIZER_LOOP
        };

        enum LedState
        {
            OFF,
            ON,
            TOGGLE,
            BLINK_ON,
            BLINK_OFF,
            DIM_LOW,
            DIM_MED,
            DIM_HIGH
        };

        TouchChannel(
            int _index,
            SuperClock *clock_ptr,
            Display *display_ptr,
            MPR121 *touchPads_ptr,
            SX1509 *leds,
            Degrees *degrees,
            DAC8554 *dac,
            DAC8554::Channel dac_chan,
            Bender *_bender,
            PinName adc_pin,
            PinName gatePin,
            DigitalOut *global_gate_ptr) : gateOut(gatePin, 0), adc(adc_pin), output(dac, dac_chan, &adc), sequence(_bender)
        {
            channelIndex = _index;
            clock = clock_ptr;
            display = display_ptr;
            touchPads = touchPads_ptr;
            _leds = leds;
            degreeSwitches = degrees;
            bender = _bender;
            globalGateOut = global_gate_ptr;
            uiMode = UIMode::UI_PLAYBACK;
            playbackMode = PlaybackMode::MONO;       // assigned from flash after init
            currBenderMode = BenderMode::PITCH_BEND; // assigned from flash after init
            currDegree = 0;
            currOctave = 0;
            
            activeDegrees = 0xFF;
            activeOctaves = 0xF;
            numActiveDegrees = DEGREE_COUNT;
            numActiveOctaves = OCTAVE_COUNT;
        };

        int channelIndex;          // an index value used for accessing the odd array
        SuperClock *clock;
        Display *display;
        MPR121 *touchPads;
        SX1509 *_leds;
        Degrees *degreeSwitches;
        Bender *bender;
        DigitalOut *globalGateOut; // global gate output
        DigitalOut gateOut; // gate output
        AnalogHandle adc;   // CV input ADC
        VoltPerOctave output;

        bool led_state[16];

        uint8_t currRatchetRate;
        uint8_t prevRatchetRate;

        bool gateState;               // state of the gate output

        UIMode uiMode;
        PlaybackMode playbackMode;

        int currBenderMode;
        int prevBenderMode;
        bool benderOverride;  // boolean which is used to disable the bender playback events (for menues)

        uint8_t currDegree;
        uint8_t currOctave;
        uint8_t prevDegree;
        uint8_t prevOctave;

        bool freezeChannel;
        int freezeStep;    // the position of sequence when freeze was enabled

        // Quantiser members
        uint8_t activeDegrees;             // 8 bits to determine which scale degrees are presently active/inactive (active = 1, inactive= 0)
        uint8_t activeOctaves;         // 4-bits to represent the current octaves external CV will get mapped to (active = 1, inactive= 0)
        int numActiveDegrees;              // number of degrees which are active (to quantize voltage input)
        int numActiveOctaves;              // number of active octaves for mapping CV to
        int activeDegreeLimit;             // the max number of degrees allowed to be enabled at one time.
        QuantDegree activeDegreeValues[8]; // array which holds noteIndex values and their associated DAC/1vo values
        QuantOctave activeOctaveValues[OCTAVE_COUNT];

        SuperSeq sequence;

        void init();
        void poll();
        void handleClock();
        void setUIMode(UIMode targetMode);
        void setPlaybackMode(PlaybackMode targetMode);
        void toggleMode();

        void handleTouchInterrupt();
        void handleTouchUIEvent(uint8_t pad);
        void handleTouchPlaybackEvent(uint8_t pad);
        void handleReleasePlaybackEvent(uint8_t pad);
        void onTouch(uint8_t pad);
        void onRelease(uint8_t pad);
        void triggerNote(int degree, int octave, Action action);
        void freeze(bool state);
        void updateDegrees();

        // UI Handler
        void updateUI(UIMode mode);

        void updateOctaveLeds(int octave, bool isPlaybackEvent);

        void setLED(int io_pin, LedState state, bool isPlaybackEvent); // if you use a "scene" here, you could remove the boolean
        void setDegreeLed(int degree, LedState state, bool isPlaybackEvent);
        void setAllDegreeLeds(LedState state, bool isPlaybackEvent);
        void setOctaveLed(int octave, LedState state, bool isPlaybackEvent);
        void setAllOctaveLeds(LedState state, bool isPlaybackEvent);

        // Display Methods
        void displayProgressCallback(uint16_t progress);

        // Quantizer methods
        void initQuantizer();
        void handleCVInput();
        void setActiveDegreeLimit(int value);
        void setActiveDegrees(uint8_t degrees);
        void setActiveOctaves(int octave);

        // Sequencer methods
        void handleSequence(int position);
        void handleRecordOverflow();
        void resetSequence();
        void updateSequenceLength(uint8_t steps);
        void setSequenceLED(uint8_t step, uint8_t pwm, bool blink);
        void drawSequenceToDisplay(bool blink);
        void stepSequenceLED();
        void enableSequenceRecording();
        void disableSequenceRecording();
        void handleQuantAmountLEDs();
        void handlePreRecordEvents();

        // Bender methods
        void handleBend(uint16_t value);
        void handlePitchBend(uint16_t value);
        int setBenderMode(BenderMode targetMode = INCREMENT_BENDER_MODE);
        void enableBenderOverride();
        void disableBenderOverride();
        void benderActiveCallback(uint16_t value);
        void benderIdleCallback();
        void benderTriStateCallback(Bender::BendState state);

        // Gate Output Methods
        void setGate(bool state);
        uint8_t calculateRatchet(uint16_t bend);
        void handleRatchet(int position, uint16_t value);

        void copyConfigData(uint32_t *arr);
        void loadConfigData(uint32_t *arr);
        
        void initializeCalibration();
        
        void logPeripherals();
    };
} // end namespace