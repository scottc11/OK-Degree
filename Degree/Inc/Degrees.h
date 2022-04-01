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

        Degrees(PinName ioIntPin, MCP23017 *io_ptr) : ioInterupt(ioIntPin)
        {
            io = io_ptr;
        };

        void init();

        void enableInterrupt();

        void handleInterrupt();

        void attachCallback(Callback<void()> func);

        void updateDegreeStates();

    private:
        enum SwitchPosition
        {
            SWITCH_UP = 2,      // 0b00000010
            SWITCH_NEUTRAL = 3, // 0b00000011
            SWITCH_DOWN = 1,    // 0b00000001
        };
    };

}