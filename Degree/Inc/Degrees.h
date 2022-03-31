#pragma once

#include "main.h"
#include "InterruptIn.h"
#include "Callback.h"
#include "MCP23017.h"

namespace DEGREE {

    class Degrees
    {
    public:
        MCP23017 *io;
        InterruptIn ioInterupt;
        Callback<void()> onChangeCallback;
        uint16_t currState;
        uint16_t prevState;

        int switchStates[8]; // holds the current position of all 8 toggle switchs in the form of a 0, 1, or 2

        Degrees(PinName ioIntPin, MCP23017 *io_ptr) : ioInterupt(ioIntPin, PullUp)
        {
            io = io_ptr;
        };

        void init()
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

            updateDegreeStates(); // get current state of toggle switches
        };

        void enableInterrupt() {
            ioInterupt.fall(callback(this, &Degrees::handleInterrupt));
        }

        void handleInterrupt()
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            uint8_t isr_id = ISR_ID_TOGGLE_SWITCHES;
            xQueueSendFromISR(qhInterruptQueue, &isr_id, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        };

        void attachCallback(Callback<void()> func)
        {
            onChangeCallback = func;
        }

        void updateDegreeStates()
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

    private:
        enum SwitchPosition
        {
            SWITCH_UP = 2,      // 0b00000010
            SWITCH_NEUTRAL = 3, // 0b00000011
            SWITCH_DOWN = 1,    // 0b00000001
        };
    };

}