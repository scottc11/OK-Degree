#pragma once

#include "main.h"
#include "Degrees.h"
#include "TouchChannel.h"
#include "Callback.h"
#include "Flash.h"

namespace DEGREE {

    class GlobalControl
    {
    public:
        GlobalControl(TouchChannel *chanA_ptr,
                      TouchChannel *chanB_ptr,
                      TouchChannel *chanC_ptr,
                      TouchChannel *chanD_ptr,
                      Degrees *degrees)
        {
            _channels[0] = chanA_ptr;
            _channels[1] = chanB_ptr;
            _channels[2] = chanC_ptr;
            _channels[3] = chanD_ptr;
            _degrees = degrees;
        };

        TouchChannel *_channels[NUM_DEGREE_CHANNELS];
        Degrees *_degrees;

        void init();
        void poll();
        void handleDegreeChange();
        void loadCalibrationDataFromFlash();

    private:
        /* data */
    };
}
