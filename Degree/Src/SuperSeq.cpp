#include "SuperSeq.h"

void SuperSeq::init()
{
    this->setLength(DEFAULT_SEQ_LENGTH);
    this->setQuantizeAmount(QUANT_8th);
    this->clearAllEvents();
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
 * @brief get the position of the given position - 1
 * 
 * @param position 
 * @return int 
 */
int SuperSeq::getPrevPosition(int position)
{
    if (position == 0) {
        return lengthPPQN - 1;
    } else {
        return position - 1;
    }
}

int SuperSeq::getLastPosition() {
    return lengthPPQN - 1;
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
 * @brief determines if the sequence contains either touch or bend events
 * 
 * @return true 
 * @return false 
 */
bool SuperSeq::containsEvents() {
    return containsBendEvents || containsTouchEvents ? true : false;
}

/**
 * @brief Deactivate all events in event array and set empty flag
*/
void SuperSeq::clearAllEvents()
{
    for (int i = 0; i < PPQN * MAX_SEQ_LENGTH; i++)
    {
        clearTouchAtPosition(i);
        clearBendAtPosition(i);
    }
    containsBendEvents = false;
    containsTouchEvents = false;
};

void SuperSeq::clearAllTouchEvents()
{
    for (int i = 0; i < PPQN * MAX_SEQ_LENGTH; i++)
    {
        clearTouchAtPosition(i);
    }
    containsTouchEvents = false;
}

void SuperSeq::clearAllBendEvents() {
    for (int i = 0; i < PPQN * MAX_SEQ_LENGTH; i++)
    {
        clearBendAtPosition(i);
    }
    containsBendEvents = false;
}

/**
 * @brief clear all bend events in sequence
 */
void SuperSeq::clearBendAtPosition(int position)
{
    events[position].bend = BENDER_DAC_ZERO;
};

/**
 * @brief clear an event in event array at given position
 * @param position sequence position (ie. index)
 */
void SuperSeq::clearTouchAtPosition(int position)
{
    events[position].data = 0x00;
}

/**
 * @brief copy the contents of an event from one position to a new position
 * 
 * @param prevPosition 
 * @param newPosition 
 */
void SuperSeq::copyPaste(int prevPosition, int newPosition)
{
    events[newPosition].data = events[prevPosition].data;
}

/**
 * @brief copy the contents of an event from one position to a new position
 *
 * @param prevPosition
 * @param newPosition
 */
void SuperSeq::cutPaste(int prevPosition, int newPosition)
{
    this->copyPaste(prevPosition, newPosition);
    clearTouchAtPosition(prevPosition);
}

/**
 * @brief Create new event at position
*/
void SuperSeq::createTouchEvent(int position, int degree, bool gate)
{
    if (!containsTouchEvents)
        containsTouchEvents = true;

    if (overdub)
    {
        // check if the last trigger event was a gate HIGH event
        if (events[prevEventPos].getGate())
        {
            // move that events associated gate LOW event one pulse before new events position
            // there is a potential bug if by chance the prev position returns an index associated with an active HIGH event
            setEventData(this->getPrevPosition(position), events[prevEventPos].getDegree(), false, true);
        }
    }
    
    newEventPos = position;
    setEventData(newEventPos, degree, gate, true);
};

/**
 * @brief set the bend value at a certain position in the events array
 * 
 * @param position 
 * @param bend 
 */
void SuperSeq::createBendEvent(int position, uint16_t bend)
{
    if (!containsBendEvents)
        containsBendEvents = true;

    events[position].bend = bend;
}

void SuperSeq::createChordEvent(int position, uint8_t degrees)
{
    if (!containsTouchEvents)
        containsTouchEvents = true;

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
int SuperSeq::getQuantizedPosition(int pos, QuantizeAmount target)
{
    int ppqnPos = pos % PPQN;
    int stepPosition = pos - ppqnPos;
    int newPosition;
    switch (target)
    {
    case QUANT_NONE:
        newPosition = ppqnPos;
        break;
    case QUANT_Quarter:
        newPosition = arr_find_closest_int((int *)Q_QUARTER_NOTE, 2, ppqnPos);
        break;
    case QUANT_8th:
        newPosition = arr_find_closest_int((int *)Q_EIGTH_NOTE, 3, ppqnPos);
        break;
    case QUANT_16th:
        newPosition = arr_find_closest_int((int *)Q_SIXTEENTH_NOTE, 4, ppqnPos);
        break;
    case QUANT_32nd:
        newPosition = arr_find_closest_int((int *)Q_THIRTY_SECOND_NOTE, 9, ppqnPos);
        break;
    case QUANT_64th:
        newPosition = arr_find_closest_int((int *)Q_SIXTY_FOURTH_NOTE, 12, ppqnPos);
        break;
    case QUANT_128th:
        newPosition = arr_find_closest_int((int *)Q_ONE_28TH_NOTE, 33, ppqnPos);
        break;
    default:
        newPosition = ppqnPos;
        break;
    }
    // check if stepPosition + newPosition does not exceed lengthPPQN
    if (stepPosition + newPosition >= lengthPPQN)
    {
        return 0; // start of seq
    } else {
        return stepPosition + newPosition;
    }
};

/**
 * @brief Quantize the current sequence
 */
void SuperSeq::quantize()
{
    logger_log("\nPRE-QUANTIZATION");
    logSequenceToConsole();
    
    int pos = 0;
    int lastGateHighPos = 0;
    int lastGateLowPos = 0;
    
    while (pos < this->lengthPPQN)
    {
        // should you keep track of gate HIGH and gate LOW events?
        // If you hold the degree index between iterations than you can delete stray gate low events 
        // as you iterate over the sequence events
        
        // If there is touch event data at this position (bit 5)
        if (events[pos].getStatus())
        {
            // if event is a gate HIGH event
            if (events[pos].getGate())
            {
                int newPos = getQuantizedPosition(pos, quantizeAmount);

                if (newPos == pos) // alread perfect, move on.
                {
                    lastGateHighPos = newPos;
                    pos++;
                    continue;
                }

                if (events[newPos].getStatus()) // is there an active event at the new position?
                {
                    if (events[newPos].getGate()) // is it a gate HIGH event?
                    {
                        this->cutPaste(pos, newPos); // overwrite that event
                    }
                    else // if the event is a gate low event
                    {
                        if (eventsAreAssociated(newPos, pos) && !events[getNextPosition(newPos)].getStatus()) // reposition gate low event to newPos + 1 (only if there isn't already an event there)
                        {
                            cutPaste(newPos, getNextPosition(newPos));
                        }
                        else // copy that gate low event to newPos - 1
                        {                            
                            this->copyPaste(newPos, getPrevPosition(newPos));
                        }
                        // copy only the touch event data to the new position, and delete it from its original position
                        this->cutPaste(pos, newPos);
                    }
                } else { // cut this event to its new position
                    cutPaste(pos, newPos);
                }
                lastGateHighPos = newPos;
            }
            else // if event is a gate LOW event
            {
                // check to see if its associated gate HIGH event has overlapped
                if (events[pos].getDegree() == events[lastGateHighPos].getDegree() && events[lastGateHighPos].getGate() == HIGH)
                {
                    if (lastGateHighPos >= pos) // if greater or equal
                    {
                        int nextPos = getNextPosition(lastGateHighPos);
                        if (events[nextPos].getStatus() == false) // if there already is an ective HIGH event, then what?
                        {
                            // move this gate low event to lastGateHighPos + 1
                            cutPaste(pos, nextPos);
                        } else {
                            clearTouchAtPosition(pos);
                        }
                    }
                }
                lastGateLowPos = pos;
            }
        }
        pos++;
    }
    logger_log("\n\nPOST-QUANTIZATION");
    logSequenceToConsole();
    logger_log("\n");
}

void SuperSeq::quantizationTest()
{
    setLength(8); // set sequence length to two steps, ie. 96 * 2 PPQN
    setQuantizeAmount(QuantizeAmount::QUANT_8th);

    createTouchEvent(370, 7, true);
    createTouchEvent(387, 7, false);
    createTouchEvent(388, 6, true);
    createTouchEvent(389, 6, false);
    createTouchEvent(400, 6, false);
    createTouchEvent(401, 5, true);
    createTouchEvent(413, 4, true);
    createTouchEvent(426, 3, true);
    createTouchEvent(439, 2, true);
    createTouchEvent(451, 2, false);
    createTouchEvent(452, 1, true);
    createTouchEvent(466, 0, true);

    quantize();
}

void SuperSeq::logSequenceToConsole() {
    logger_log("\nlengthPPQN: ");
    logger_log(lengthPPQN);
    logger_log(", Quant: ");
    logger_log((int)quantizeAmount);
    logger_log("\n||  pos  |  degree  |  gate  ||");
    for (int pos = 0; pos < lengthPPQN; pos++)
    {
        if (events[pos].getStatus())
        {
            logger_log("\n||  ");
            logger_log(pos);
            logger_log("  |    ");
            logger_log(events[pos].getDegree());
            logger_log("    |  ");
            logger_log(events[pos].getGate());
            logger_log("  ||");
        }
    }
}

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
    if (gate == LOW) // avoid overwriting any active HIGH event with a active LOW event
    {
        if (events[position].getStatus() && events[position].getGate())
        {
            return;
        }
    }
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

bool SuperSeq::eventsAreAssociated(int pos1, int pos2)
{
    if (events[pos1].getDegree() == events[pos2].getDegree())
    {
        return true;
    } else {
        return false;
    }
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
    return state ? bitwise_set_bit(byte, SEQ_EVENT_GATE_BIT) : bitwise_clear_bit(byte, SEQ_EVENT_GATE_BIT);
}

uint8_t SuperSeq::readGateBits(uint8_t byte)
{
    return bitwise_read_bit(byte, SEQ_EVENT_GATE_BIT);
}

uint8_t SuperSeq::setStatusBits(bool status, uint8_t byte)
{
    return status ? bitwise_set_bit(byte, SEQ_EVENT_STATUS_BIT) : bitwise_clear_bit(byte, SEQ_EVENT_STATUS_BIT);
}

uint8_t SuperSeq::readStatusBits(uint8_t byte)
{
    return bitwise_read_bit(byte, SEQ_EVENT_STATUS_BIT);
}