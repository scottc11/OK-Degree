#pragma once

#include "main.h"
#include "logger.h"
#include "TouchChannel.h"
#include "PitchFrequencies.h"

QueueHandle_t tuner_queue;
TaskHandle_t tuner_task_handle;

using namespace DEGREE;

void task_tuner(void *params);