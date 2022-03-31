#include "Degrees.h"

using namespace DEGREE;

void Degrees::init()
{
    io->init();
    io->setDirection(MCP23017_PORTA, 0xFF);     // set PORTA pins as inputs
    io->setDirection(MCP23017_PORTB, 0xFF);     // set PORTB pins as inputs
    io->setPullUp(MCP23017_PORTA, 0xFF);        // activate PORTA pin pull-ups for toggle switches
    io->setPullUp(MCP23017_PORTB, 0xFF);        // activate PORTB pin pull-ups for toggle switches
    io->setInputPolarity(MCP23017_PORTA, 0x00); // invert PORTA pins input polarity for toggle switches
    io->setInputPolarity(MCP23017_PORTB, 0x00); // invert PORTB pins input polarity for toggle switches
    io->setInterupt(MCP23017_PORTA, 0xFF);
    io->setInterupt(MCP23017_PORTB, 0xFF);
};

void Degrees::enableInterrupt()
{
    ioInterupt.fall(callback(this, &Degrees::handleInterrupt));
}

void Degrees::handleInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t isr_id = ISR_ID_TOGGLE_SWITCHES;
    xQueueSendFromISR(qhInterruptQueue, &isr_id, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
};

void Degrees::attachCallback(Callback<void()> func)
{
    onChangeCallback = func;
}

void Degrees::updateDegreeStates()
{
    currState = io->digitalReadAB();
    if (currState != prevState)
    {
        int switchIndex = 0;
        for (int i = 0; i < 16; i++)
        { // iterate over all 16 bits
            if (i % 2 == 0)
            { // only checking bits in pairs
                int bitA = io->getBitStatus(currState, i);
                int bitB = io->getBitStatus(currState, i + 1);
                int value = (bitB | bitA) >> i;
                switch (value)
                {
                case SWITCH_UP:
                    switchStates[switchIndex] = 2;
                    break;
                case SWITCH_NEUTRAL:
                    switchStates[switchIndex] = 1;
                    break;
                case SWITCH_DOWN:
                    switchStates[switchIndex] = 0;
                    break;
                }
                switchIndex += 1;
            }
        }
        prevState = currState;
        if (onChangeCallback)
        {
            onChangeCallback();
        }
    }
};