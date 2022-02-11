#include "SuperSeq.h"

void SuperSeq::init()
{
    this->setLength(DEFAULT_SEQ_LENGTH);
    this->setQuantizeAmount(QUANT_NONE);
    this->clear();
};

/**
 * @brief will return position + 1 as long as it isn't the last pulse in the sequence
*/
int SuperSeq::getNextPosition(int position)
{
    if (position + 1 < lengthPPQN)
    {
        return position + 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief advance the sequencer position by 1
 * @note you cant make any I2C calls in these functions, you must defer them to a seperate thread to be executed later
*/
void SuperSeq::advance()
{
    prevPosition = currPosition;
    currPosition += 1;
    currStepPosition += 1;

    if (currPosition % PPQN == 0)
    {
        prevStep = currStep;
        currStep += 1;
        currStepPosition = 0;
    }

    if (currPosition > lengthPPQN - 1)
    {
        currPosition = 0;
        currStep = 0;
    }
}

void SuperSeq::advanceStep()
{
    currPosition = (currPosition - (currPosition % (PPQN - 1))) + (PPQN - 1);
}

/**
 * @brief reset the sequence
*/ 
void SuperSeq::reset()
{
    prevPosition = currPosition;
    currPosition = 0;
    prevStep = currStep;
    currStep = 0;
};

/**
 * @brief reset the current position to the position equal to the last step
*/ 
void SuperSeq::resetStep()
{
    currPosition = (currPosition - (currPosition % (PPQN - 1))) - (PPQN - 1);
}

/**
 * @brief Deactivate all events in event array and set empty flag
*/
void SuperSeq::clear()
{
    
    for (int i = 0; i < PPQN * MAX_SEQ_LENGTH; i++)
    {
        clearEvent(i);
    }
    containsEvents = false; // after deactivating all events in list, set this flag to false
};

/**
 * @brief clear all bend events in sequence
 */
void SuperSeq::clearBend()
{
    // deactivate all events in list
    for (int i = 0; i < PPQN * MAX_SEQ_LENGTH; i++)
    {
        events[i].bend = bender->getIdleValue();
    }
};

/**
 * @brief clear an event in event array at given position
 * @param position sequence position (ie. index)
 */
void SuperSeq::clearEvent(int position)
{
    events[position].data = 0x00;
    events[position].bend = bender->getIdleValue();
}

/**
 * @brief Create new event at position
*/
void SuperSeq::createEvent(int position, int noteIndex, bool gate)
{
    if (containsEvents == false)
    {
        containsEvents = true;
    }

    // handle quantization first, for overdubbing purposes
    // int quantizedPosition = (currStep * PPQN) + quantizePosition(position, quantizeAmount);
    int quantizedPosition = position;

    if (quantizedPosition > position) {
        // flag the sequence handler to not trigger this event, as it has already been triggered manually
        // one way to do this: disable ALL new events until the sequence has come back around.
        // or... just don't quantize on the fly
    }
    

    // if the previous event was a GATE HIGH event, re-position its succeeding GATE LOW event to the new events position - 1
    // NOTE: you will also have to trigger the GATE LOW, so that the new event will generate a trigger event
    // TODO: intsead of "position - 1", do "position - (quantizeAmount / 2)"
    if (getEventGate(prevEventPos) == HIGH)
    {
        int newPosition = quantizedPosition == 0 ? lengthPPQN - 1 : quantizedPosition - 1;
        setEventData(newPosition, getEventDegree(prevEventPos), LOW, true);
        prevEventPos = newPosition; // pretend like this event got executed
    }

    // if this new event is a GATE LOW event, and after quantization there exists an event at the same position in the sequence,
    // move this new event to the next available position @ the curr quantize level divided by 2 (to not interfere with next event), which will also have to be checked for any existing events.
    // If there is an existing GATE HIGH event at the next position, this new event will have to be placed right before it executes, regardless of quantization
    if (gate == LOW && getEventStatus(quantizedPosition) && getEventGate(quantizedPosition) == HIGH)
    {
        int move = quantizeAmount == 0 ? PPQN / 2 : quantizeAmount / 2;
        quantizedPosition = quantizedPosition + (move);
    }

    newEventPos = quantizedPosition; // store new events position
    setEventData(newEventPos, noteIndex, gate, true);
};

void SuperSeq::createBendEvent(int position, uint16_t bend)
{
    if (containsEvents == false)
    {
        containsEvents = true;
    }

    events[position].bend = bend;
}

void SuperSeq::createChordEvent(int position, uint8_t degrees)
{
    if (containsEvents == false)
    {
        containsEvents = true;
    }

    events[position].activeDegrees = degrees;
    setEventStatus(position, true);
};

/**
 * @brief Set how many steps a sequence will contain (not PPQN)
*/ 
void SuperSeq::setLength(int steps)
{
    if (steps > 0 && steps <= MAX_SEQ_LENGTH)
    {
        length = steps;
        lengthPPQN = length * PPQN;
    }
};

/**
 * @brief set the level new events get quantized too
*/ 
void SuperSeq::setQuantizeAmount(QuantizeAmount value)
{
    quantizeAmount = value;
}

const int Q_QUARTER_NOTE[2] = {0, 96};
const int Q_EIGTH_NOTE[3] = {0, 48, 96};
const int Q_SIXTEENTH_NOTE[4] = {0, 24, 48, 96};
const int Q_THIRTY_SECOND_NOTE[9] = {0, 12, 24, 36, 48, 60, 72, 84, 96};
const int Q_SIXTY_FOURTH_NOTE[12] = {0, 6, 12, 18, 24, 30, 36, 72, 78, 84, 90, 96};
const int Q_ONE_28TH_NOTE[33] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96};

/**
 * @brief snap input position to a quantized grid
*/
int SuperSeq::quantizePosition(int pos, QuantizeAmount target)
{
    int newPosition;
    switch (target)
    {
    case QUANT_NONE:
        newPosition = pos;
        break;
    case QUANT_Quarter:
        newPosition = arr_find_closest_int((int *)Q_QUARTER_NOTE, 2, pos);
        break;
    case QUANT_8th:
        newPosition = arr_find_closest_int((int *)Q_EIGTH_NOTE, 3, pos);
        break;
    case QUANT_16th:
        newPosition = arr_find_closest_int((int *)Q_SIXTEENTH_NOTE, 4, pos);
        break;
    case QUANT_32nd:
        newPosition = arr_find_closest_int((int *)Q_THIRTY_SECOND_NOTE, 9, pos);
        break;
    case QUANT_64th:
        newPosition = arr_find_closest_int((int *)Q_SIXTY_FOURTH_NOTE, 12, pos);
        break;
    case QUANT_128th:
        newPosition = arr_find_closest_int((int *)Q_ONE_28TH_NOTE, 33, pos);
        break;
    default:
        newPosition = pos;
        break;
    }
    return newPosition;
};

/**
 * @brief contruct an 8-bit value which holds the degree index, gate state, and status of a sequence event.
 *
 * @param degree the degree index
 * @param gate the state of the gate output (high or low)
 * @param status the status of the event
 * @return uint8_t
 */
uint8_t SuperSeq::constructEventData(uint8_t degree, bool gate, bool status)
{
    uint8_t data = 0x00;
    data = setIndexBits(degree, data);
    data = setGateBits(gate, data);
    data = setStatusBits(status, data);
    return data;
}

void SuperSeq::setEventData(int position, uint8_t degree, bool gate, bool status)
{
    events[position].data = constructEventData(degree, gate, status);
}

uint8_t SuperSeq::getEventDegree(int position)
{
    return readDegreeBits(events[position].data);
}

uint8_t SuperSeq::getActiveDegrees(int position)
{
    return events[position].activeDegrees;
}

bool SuperSeq::getEventGate(int position)
{
    return readGateBits(events[position].data);
}

bool SuperSeq::getEventStatus(int position)
{
    return readStatusBits(events[position].data);
}

void SuperSeq::setEventStatus(int position, bool status)
{
    events[position].data = setStatusBits(status, events[position].data);
}

uint16_t SuperSeq::getBend(int position) {
    return events[position].bend;
}

uint8_t SuperSeq::setIndexBits(uint8_t degree, uint8_t byte)
{
    return byte | degree;
}

uint8_t SuperSeq::readDegreeBits(uint8_t byte)
{
    return byte & SEQ_EVENT_INDEX_BIT_MASK;
}

uint8_t SuperSeq::setGateBits(bool state, uint8_t byte)
{
    return state ? bitSet(byte, SEQ_EVENT_GATE_BIT) : bitClear(byte, SEQ_EVENT_GATE_BIT);
}

uint8_t SuperSeq::readGateBits(uint8_t byte)
{
    return bitRead(byte, SEQ_EVENT_GATE_BIT);
}

uint8_t SuperSeq::setStatusBits(bool status, uint8_t byte)
{
    return status ? bitSet(byte, SEQ_EVENT_STATUS_BIT) : bitClear(byte, SEQ_EVENT_STATUS_BIT);
}

uint8_t SuperSeq::readStatusBits(uint8_t byte)
{
    return bitRead(byte, SEQ_EVENT_STATUS_BIT);
}