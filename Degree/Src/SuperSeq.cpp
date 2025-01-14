#include "SuperSeq.h"

void SuperSeq::init()
{
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
        if (recordEnabled && adaptiveLength)
        {
            if (currStep >= MAX_SEQ_LENGTH) // disable adaptive length, set seq length to max
            {
                adaptiveLength = false;
                this->setLength(MAX_SEQ_LENGTH);
                currPosition = 0;
                currStep = 0;
            } else {
                // do nothing, let things keep counting upwards
            }
        }
        else // reset loop
        {
            currPosition = 0;
            currStep = 0;
        }
    }
}

void SuperSeq::enableRecording() {
    this->recordEnabled = true;
    // if no currently recorded events, enable adaptive length
    if (!this->containsEvents()) {
        this->reset();
        this->setLength(2);
        this->adaptiveLength = true;
    } else {
        this->adaptiveLength = false;
    }
}

void SuperSeq::disableRecording() {
    this->recordEnabled = false;
    
    // if events were recorded and adaptive length was enabled, update the seq length
    if (adaptiveLength)
    {
        this->adaptiveLength = false;

        if (this->containsEvents())
        {
            // is all this zero indexed?
            if (this->currStep <= SEQ_LENGTH_BLOCK_1)
            {
                this->setLength(SEQ_LENGTH_BLOCK_1); // 8 steps
            }
            else if (this->currStep > SEQ_LENGTH_BLOCK_1 && this->currStep <= SEQ_LENGTH_BLOCK_2)
            {
                this->setLength(SEQ_LENGTH_BLOCK_2);
            }
            else if (this->currStep > SEQ_LENGTH_BLOCK_2 && this->currStep <= SEQ_LENGTH_BLOCK_3)
            {
                this->setLength(SEQ_LENGTH_BLOCK_3);
            }
            else if (this->currStep > SEQ_LENGTH_BLOCK_3)
            {
                this->setLength(SEQ_LENGTH_BLOCK_4);
            }
        }
    }
}

void SuperSeq::enablePlayback()
{
    playbackEnabled = true;
}

void SuperSeq::disablePlayback()
{
    playbackEnabled = false;
}

void SuperSeq::enableOverdub()
{
    overdub = true;
}

void SuperSeq::disableOverdub()
{
    overdub = false;
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
void SuperSeq::createTouchEvent(int position, uint8_t degree, uint8_t octave, bool gate)
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
            setEventData(this->getPrevPosition(position), events[prevEventPos].getDegree(), events[prevEventPos].getOctave(), false, true);
        }
    }
    
    newEventPos = position;
    setEventData(newEventPos, degree, octave, gate, true);
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

void SuperSeq::createChordEvent(int position, uint8_t degrees, uint8_t octaves)
{
    if (!containsTouchEvents)
        containsTouchEvents = true;

    events[position].activeDegrees = degrees;
    events[position].data = octaves;
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
void SuperSeq::setQuantizeAmount(QUANT value)
{
    quantizeAmount = value;
}

/**
 * @brief Quantize the current sequence
 */
void SuperSeq::quantize()
{
    // logger_log("\nPRE-QUANTIZATION");
    // logSequenceToConsole();
    
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
                int newPos = getQuantizedPosition(pos, lengthPPQN, quantizeAmount);

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
    // logger_log("\n\nPOST-QUANTIZATION");
    // logSequenceToConsole();
    // logger_log("\n");
}

void SuperSeq::logSequenceToConsole() {
    logger_log("\nlengthPPQN: ");
    logger_log(lengthPPQN);
    logger_log(", Quant: ");
    logger_log((int)quantizeAmount);
    logger_log("\n||  POS  |  DEG  |  GATE  ||");
    for (int pos = 0; pos < lengthPPQN; pos++)
    {
        if (events[pos].getStatus())
        {
            logger_log("\n*|  ");
            logger_log(pos);
            logger_log("  |   ");
            logger_log(events[pos].getDegree());
            logger_log("   |  ");
            logger_log(events[pos].getGate());
            logger_log("  |*");
        }
    }
}

/**
 * @brief contruct an 8-bit value which holds the degree index, octave, gate state, and status of a sequence event.
 *
 * @param degree the degree index (0..7)
 * @param octave the octave (0..3)
 * @param gate the state of the gate output (high or low)
 * @param status the status of the event
 */
void SuperSeq::setEventData(int position, uint8_t degree, uint8_t octave, bool gate, bool status)
{
    if (gate == LOW) // avoid overwriting any active HIGH event with a active LOW event
    {
        if (events[position].getStatus() && events[position].getGate())
        {
            return;
        }
    }
    uint8_t data = 0b00000000;
    data = setIndexBits(degree, data);
    data = setGateBits(gate, data);
    data = setStatusBits(status, data);
    data = setOctaveBits(octave, data);
    events[position].data = data;
}

/**
 * @brief combine all event struct data into a 32-bit number
 * 
 * @param position 
 * @return uint32_t 
 */
uint32_t SuperSeq::encodeEventData(int position)
{
    uint32_t data;
    data = events[position].bend;                      // 16-bits
    data = (data << 8) | events[position].activeDegrees; // 8-bits
    data = (data << 8) | events[position].data;          // 8-bits
    return data;
}

/**
 * @brief unpack event data and store into event struct
 * 
 * @param position 
 * @param data data chunk from flash
 */
void SuperSeq::decodeEventData(int position, uint32_t data)
{
    events[position].bend = (uint16_t)(data >> 16);
    events[position].activeDegrees = (uint8_t)((data & 0x0000FF00) >> 8);
    events[position].data = (uint8_t)(data & 0x000000FF);
}

/**
 * @brief store sequence configuration into an array (to be stored in flash)
 * 
 * @param arr 
 * @return uint32_t 
 */
void SuperSeq::storeSequenceConfigData(uint32_t *arr) {
    arr[0] = this->length;
    arr[1] = this->lengthPPQN;
    arr[2] = this->containsBendEvents;
    arr[3] = this->containsTouchEvents;
    arr[4] = (uint32_t)this->quantizeAmount;
}

void SuperSeq::loadSequenceConfigData(uint32_t *arr)
{
    this->length = (int)arr[0];
    this->lengthPPQN = (int)arr[1];
    this->containsBendEvents = (bool)arr[2];
    this->containsTouchEvents = (bool)arr[3];
    this->quantizeAmount = (enum QUANT)arr[4];
}

uint8_t SuperSeq::getEventDegree(int position)
{
    return events[position].getDegree();
}

uint8_t SuperSeq::getEventOctave(int position)
{
    return events[position].getOctave();
}

uint8_t SuperSeq::getActiveDegrees(int position)
{
    return events[position].activeDegrees;
}

uint8_t SuperSeq::getActiveOctaves(int position)
{
    return events[position].getActiveOctaves();
}

bool SuperSeq::getEventGate(int position)
{
    return events[position].getGate();
}

bool SuperSeq::getEventStatus(int position)
{
    return events[position].getStatus();
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

uint8_t SuperSeq::setGateBits(bool state, uint8_t byte)
{
    return state ? bitwise_set_bit(byte, SEQ_EVENT_GATE_BIT) : bitwise_clear_bit(byte, SEQ_EVENT_GATE_BIT);
}

uint8_t SuperSeq::setStatusBits(bool status, uint8_t byte)
{
    return status ? bitwise_set_bit(byte, SEQ_EVENT_STATUS_BIT) : bitwise_clear_bit(byte, SEQ_EVENT_STATUS_BIT);
}

uint8_t SuperSeq::setOctaveBits(uint8_t octave, uint8_t byte)
{
    octave = octave << 6;     // shift octave value from bits 0 and 1 to bits 7 and 8
    byte = byte & 0b00111111; // clear bits 7 and 8
    return byte | octave;     // set bits 7 and 8:
}

uint8_t SuperSeq::setActiveOctaveBits(uint8_t octaves) {
    return octaves;
}