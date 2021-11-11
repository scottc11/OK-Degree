#include "SuperSeq.h"

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

    if (currPosition % PPQN == 0)
    {
        prevStep = currStep;
        currStep += 1;
    }

    if (currPosition > lengthPPQN - 1)
    {
        currPosition = 0;
        currStep = 0;
    }
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