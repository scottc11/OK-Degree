#pragma once

#include "main.h"
#include "logger.h"
#include "TouchChannel.h"
#include "PitchFrequencies.h"

using namespace DEGREE;

void timer_callback(TimerHandle_t xTimer);
void task_tuner(void *params);