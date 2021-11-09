#pragma once

#include "main.h"
#include "Degrees.h"
#include "TouchChannel.h"
#include "Callback.h"
#include "Flash.h"
#include "MCP23017.h"

namespace DEGREE {

    class GlobalControl
    {
    public:
        GlobalControl(TouchChannel *chanA_ptr,
                      TouchChannel *chanB_ptr,
                      TouchChannel *chanC_ptr,
                      TouchChannel *chanD_ptr,
                      Degrees *degrees_ptr,
                      MCP23017 *buttons_ptr,
                      PinName io_int_pin,
                      PinName rec_led_pin,
                      PinName freeze_led_pin) : ioInterupt(io_int_pin), recLED(rec_led_pin, 0), freezeLED(freeze_led_pin, 0)
        {
            _channels[0] = chanA_ptr;
            _channels[1] = chanB_ptr;
            _channels[2] = chanC_ptr;
            _channels[3] = chanD_ptr;
            switches = degrees_ptr;
            buttons = buttons_ptr;
            ioInterupt.fall(callback(this, &GlobalControl::handleButtonInterupt)); // important to do this prior to io init
        };

        TouchChannel *_channels[NUM_DEGREE_CHANNELS];
        Degrees *switches;      // degree 3-stage toggle switches io
        MCP23017 *buttons;      // io for tactile buttons
        InterruptIn ioInterupt; // interupt pin for buttons MCP23017 io
        DigitalOut recLED;
        DigitalOut freezeLED;
        bool buttonChanged;

        void init();
        void poll();
        void handleSwitchChange();
        void handleButtonChange();
        void handleButtonInterupt();
        void loadCalibrationDataFromFlash();

    private:
        /* data */
    };
}
