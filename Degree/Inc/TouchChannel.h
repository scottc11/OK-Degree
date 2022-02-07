#pragma once

#include "main.h"
#include "MPR121.h"
#include "SX1509.h"
#include "DAC8554.h"
#include "Degrees.h"
#include "Bender.h"
#include "VoltPerOctave.h"
#include "SuperSeq.h"
#include "Display.h"
#include "okSemaphore.h"

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
        
        enum BenderMode : int
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
            OFF,
            ON,
            BLINK_ON,
            BLINK_OFF,
            DIM_LOW,
            DIM_MED,
            DIM_HIGH
        };

        TouchChannel(
            int _index,
            Display *display_ptr,
            MPR121 *touchPads_ptr,
            SX1509 *leds,
            Degrees *degrees,
            DAC8554 *dac,
            DAC8554::Channel dac_chan,
            Bender *_bender,
            PinName adc_pin,
            PinName gatePin,
            DigitalOut *global_gate_ptr) : gateOut(gatePin, 0), adc(adc_pin), output(dac, dac_chan, &adc)
        {
            channelIndex = _index;
            display = display_ptr;
            touchPads = touchPads_ptr;
            _leds = leds;
            degreeSwitches = degrees;
            bender = _bender;
            globalGateOut = global_gate_ptr;

            currMode = MONO;
            prevMode = MONO;
            benderMode = PITCH_BEND;
            currDegree = 0;
            currOctave = 0;
            
            activeDegrees = 0xFF;
            currActiveOctaves = 0xF;
            numActiveDegrees = DEGREE_COUNT;
            numActiveOctaves = OCTAVE_COUNT;
        };

        int channelIndex;          // an index value used for accessing the odd array
        Display *display;
        MPR121 *touchPads;
        SX1509 *_leds;
        Degrees *degreeSwitches;
        Bender *bender;
        DigitalOut *globalGateOut; // global gate output
        DigitalOut gateOut; // gate output
        AnalogHandle adc;   // CV input ADC
        VoltPerOctave output;        
        
        uint8_t currRatchetRate;      //
        bool gateState;               // state of the gate output
        TouchChannelMode currMode;
        TouchChannelMode prevMode;

        int benderMode;

        uint8_t currDegree;
        uint8_t currOctave;
        uint8_t prevDegree;
        uint8_t prevOctave;

        bool freezeChannel;

        // Quantiser members
        uint8_t activeDegrees;             // 8 bits to determine which scale degrees are presently active/inactive (active = 1, inactive= 0)
        uint8_t currActiveOctaves;         // 4-bits to represent the current octaves external CV will get mapped to (active = 1, inactive= 0)
        uint8_t prevActiveOctaves;         // 4-bits to represent the previous octaves external CV will get mapped to (active = 1, inactive= 0)
        int numActiveDegrees;              // number of degrees which are active (to quantize voltage input)
        int numActiveOctaves;              // number of active octaves for mapping CV to
        int activeDegreeLimit;             // the max number of degrees allowed to be enabled at one time.
        uint16_t prevCVInputValue;         // the previously handled CV input value 
        QuantDegree activeDegreeValues[8]; // array which holds noteIndex values and their associated DAC/1vo values
        QuantOctave activeOctaveValues[OCTAVE_COUNT];

        SuperSeq sequence;
        bool tickerFlag;                   // 

        void init();
        void poll();
        void setMode(TouchChannelMode targetMode);
        void toggleMode();

        void onTouch(uint8_t pad);
        void onRelease(uint8_t pad);
        void triggerNote(int degree, int octave, Action action);
        void freeze(bool state);
        void updateDegrees();

        void setOctave(int octave);
        void updateOctaveLeds(int octave);

        void setLED(int io_pin, LedState state);
        void setDegreeLed(int degree, LedState state);
        void setOctaveLed(int octave, LedState state);

        // Quantizer methods
        void initQuantizer();
        void handleCVInput();
        void setActiveDegreeLimit(int value);
        void setActiveDegrees(uint8_t degrees);
        void setActiveOctaves(int octave);

        // Sequencer methods
        void handleSequence(int position);
        void resetSequence();
        void enableSequenceRecording();
        void disableSequenceRecording();
        void setTickerFlag()   { tickerFlag = true; };
        void clearTickerFlag() { tickerFlag = false; };

        // Bender methods
        int setBenderMode(BenderMode targetMode = INCREMENT_BENDER_MODE);
        void benderActiveCallback(uint16_t value);
        void benderIdleCallback();
        void benderTriStateCallback(Bender::BendState state);

        // Gate Output Methods
        void setGate(bool state);
        uint8_t calculateRatchet(uint16_t bend);
        void handleRatchet(int position, uint8_t rate);

        void initializeCalibration();
    };
} // end namespace

/**
 * TODO: create a static member of a mutex type for each shared resource multiple channels instances might use.
 * ie. for the SPI / I2C peripherals
 *
 * An alternative to using mutexes would be to create a single "gatekeeper" task for each peripheral which listens for data entering a queue.
 * Each touch channel instance would post data to the queue
 *
 * MPR121 gatekeeper:
 *  - uses a queue which holds an 8-bit data. when a touch interupt occurs it loads the queue with a bit corrosponding to a channel, then the gatekeeper
 *    reads the corrosponding MPR121 IC
 *  - Might best use the Event Groups API of FreeRTOS ie. xEventGroupSetBitsFromISR()
 *
 * Sequencer Queue + Task:
 *  - Inside the SuperClock ISR function that increments the PPQN by 1, you would additionally flag the "Sequencer Task" to execute its code. This way the scheduler is
 *  somewhat synced to the Timer.
 *
 * Tasks:
 * 1) Touch Read
 * 2) Sequencer
 * 3) Quantizer
 * 4) VCO Calibrator
 * 5) Bender Calibrator
 */

/*
QueueHandle_t touchQueue;
TaskHandle_t touch_handle;

void taskHandleTouch(void *params)
{
  uint8_t value_received;
  BaseType_t status;
  while (1)
  {
    status = xQueueReceive(touchQueue, &value_received, portMAX_DELAY);

    if (status != pdPASS)
    {
      serial.transmit("Could not recieive data from queue\n");
    }
    else
    {
      uint16_t touched = touchA.readPads();
      serial.transmit("Recieived value from queue: ");
      serial.transmit(touched);
      serial.transmit("\n");
    }
  }
}

void touchInterupt()
{
  const uint8_t val = 2;
  xQueueSendFromISR(touchQueue, &val, pdFALSE);
  portYIELD_FROM_ISR(pdFALSE);
}
*/