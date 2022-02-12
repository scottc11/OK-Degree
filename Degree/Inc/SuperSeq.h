#pragma once

#include "main.h"
#include "Bender.h"
#include "ArrayMethods.h"

#define NULL_NOTE_INDEX 99 // used to identify a 'null' or 'deleted' sequence event
#define SEQ_EVENT_STATUS_BIT 5
#define SEQ_EVENT_GATE_BIT 4
#define SEQ_EVENT_INDEX_BIT_MASK 0b00001111

typedef struct SequenceNode
{
    uint8_t activeDegrees; // byte for holding active/inactive notes for a chord
    uint8_t data;          // bits 0..3: Degree Index || bit 4: Gate || bit 5: status
    uint16_t bend;         // raw ADC value from pitch bend
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

    SuperSeq(Bender *benderPtr) {
        bender = benderPtr;
    };

    Bender *bender;       // you need the instance of a bender for determing its idle value when clearing / initializing bender events
    QuantizeAmount quantizeAmount;

    int length;              // how many steps the sequence contains
    int lengthPPQN;          // how many PPQN the sequence contains
    int currStepPosition;    // the number of PPQN that have passed since the last step was advanced
    int currStep;            // current sequence step
    int prevStep;            // the previous step executed in the sequence
    int currPosition;        // current position of sequence (in PPQN)
    int prevPosition;
    int prevEventPos;        // represents the position of the last event which got triggered (either HIGH or LOW)
    int newEventPos;         // when a new event is created, we store the position in this variable in case we need it for something (ie. sequence overdubing)

    bool overdub;            // flag gets set to true so that the sequence handler clears/overdubs existing events
    bool recordEnabled;      // when true, sequence will create and new events to the event list
    bool playbackEnabled;    // when true, sequence will playback event list
    bool bendEnabled;        // flag used for overriding current recorded bend with active bend    
    bool containsTouchEvents;// flag indicating if a sequence has any touch events
    bool containsBendEvents; // flag indicating if a sequence has any bend events

    void init();
    void reset();
    void resetStep();
    bool containsEvents();
    void clearAllEvents();
    void clearAllTouchEvents();
    void clearAllBendEvents();
    void clearBendAtPosition(int position);
    void clearTouchAtPosition(int position);
    void createTouchEvent(int position, int noteIndex, bool gate);
    void createBendEvent(int position, uint16_t bend);
    void createChordEvent(int position, uint8_t notes);
    void setLength(int steps);
    int getLength();
    void setQuantizeAmount(QuantizeAmount value);
    int quantizePosition(int pos, QuantizeAmount target);
    int getNextPosition(int position);
    void advance();
    void advanceStep();

    uint8_t constructEventData(uint8_t degree, bool gate, bool status);
    void setEventData(int position, uint8_t degree, bool gate, bool status);

    uint8_t getEventDegree(int position);
    uint8_t getActiveDegrees(int position);
    bool getEventGate(int position);
    bool getEventStatus(int position);
    uint16_t getBend(int position);
    
    void setEventStatus(int position, bool status);

    uint8_t setIndexBits(uint8_t degree, uint8_t byte);
    uint8_t readDegreeBits(uint8_t byte);
    uint8_t setGateBits(bool state, uint8_t byte);
    uint8_t readGateBits(uint8_t byte);
    uint8_t setStatusBits(bool status, uint8_t byte);
    uint8_t readStatusBits(uint8_t byte);

private:
    SequenceNode events[PPQN * MAX_SEQ_LENGTH];
};