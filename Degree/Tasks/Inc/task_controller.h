#pragma once

#include "main.h"
#include "MultiChanADC.h"
#include "logger.h"
#include "GlobalControl.h"
#include "task_calibration.h"
#include "task_tuner.h"
#include "task_sequence_handler.h"

using namespace DEGREE;

void task_controller(void *params);