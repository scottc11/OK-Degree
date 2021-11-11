#pragma once

#include "main.h"
#include "ArrayMethods.h"

#define NULL_NOTE_INDEX 99 // used to identify a 'null' or 'deleted' sequence event

typedef struct SequenceNode
{
    uint8_t activeDegrees; // byte for holding active/inactive notes for a chord
    uint8_t noteIndex;     // note index between 0 and 7 NOTE: you could tag on some extra data in the bottom most bits, like gate on / off for example
    uint16_t bend;         // raw ADC value from pitch bend
    bool gate;             // set gate HIGH or LOW
    bool active;           // this will tell the loop whether to trigger an event or not
} SequenceNode;

class SuperSeq {
public:
    
    enum QuantizeAmount
    {
        QUANT_NONE = 0,
        QUANT_Quarter = PPQN,
        QUANT_8th = PPQN / 2,
        QUANT_16th = PPQN / 4,
        QUANT_32nd = PPQN / 8,
        QUANT_64th = PPQN / 16,
        QUANT_128th = PPQN / 32
    };

    SuperSeq(){};

    SequenceNode events[PPQN * MAX_SEQ_LENGTH];
    QuantizeAmount quantizeAmount;

    int length;       // how many steps the sequence contains
    int lengthPPQN;   // how many PPQN the sequence contains
    int currStep;     // current sequence step
    int prevStep;     // the previous step executed in the sequence
    int currPosition; // current
    int prevPosition;
    int prevEventPos;     // represents the position of the last event which got triggered (either HIGH or LOW)
    int newEventPos;      // when a new event is created, we store the position in this variable in case we need it for something (ie. sequence overdubing)
    bool overdub;         // flag gets set to true so that the sequence handler clears/overdubs existing events
    bool recordEnabled;   // when true, sequence will create and new events to the event list
    bool playbackEnabled; // when true, sequence will playback event list
    bool containsEvents;  // flag to determine when a sequence is cleared / empty

    void init();

    void reset();

    void clear();

    void clearBend();

    void clearEvent(int position);

    void createEvent(int position, int noteIndex, bool gate, QuantizeAmount quant);

    void createBendEvent(int position, uint16_t bend);

    void createChordEvent(int position, uint8_t notes);

    void setLength(int steps);

    int getLength();

    void setQuantizeAmount(QuantizeAmount value);

    int quantizePosition(int pos, QuantizeAmount target);

    int getNextPosition(int position);

    void advance();

    /**
     * @brief advance the sequencer to a specific step
    */
    void advanceToStep(int step) {

    };
};