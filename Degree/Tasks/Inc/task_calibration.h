#pragma once

#include "main.h"
#include "Algorithms.h"
#include "TouchChannel.h"
#include "GlobalControl.h"
#include "MultiChanADC.h"
#include "logger.h"
#include "ArrayMethods.h"
#include "PitchFrequencies.h"

void taskObtainSignalFrequency(void *params);
void taskCalibrate(void *params);