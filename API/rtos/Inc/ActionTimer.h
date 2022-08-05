#pragma once

#include "cmsis_os.h"
#include "Callback.h"
#include "SoftwareTimer.h"

class ActionTimer {
public:
    ActionTimer() {};

    SoftwareTimer timer;
    Callback<void(int progress)> progressCallback;
    Callback<void()> completeCallback;

    void start();
    void stop();

    void reset() {
        counter = 0;
    }

private:
    int counter;
    int counterLimit;
};